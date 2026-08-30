#include "bmp388.h"
#include <math.h>

/* Static calibration data */
static BMP388_CalibTypeDef bmp388_calib;

/* pressure parameters */ 
static float bmp388_reference_pressure = 0.0f;


/* Internal function prototypes */

static BMP388_StatusTypeDef BMP388_ReadRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data, uint16_t len);
static BMP388_StatusTypeDef BMP388_WriteRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t value);
static BMP388_StatusTypeDef BMP388_Config(I2C_HandleTypeDef *hi2c);

static BMP388_StatusTypeDef BMP388_ReadRaw(I2C_HandleTypeDef *hi2c, uint32_t *raw_press,uint32_t *raw_temp);
static BMP388_StatusTypeDef BMP388_ReadCalibration(I2C_HandleTypeDef *hi2c);


static float BMP388_CompensateTemperature(uint32_t adc_temp);
static float BMP388_CompensatePressure(uint32_t adc_press);

static float BMP388_CalculateAltitude(float pressure_pa);

/* ================= LOW LEVEL ================= */

static BMP388_StatusTypeDef BMP388_ReadRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data, uint16_t len)
{
      if (HAL_I2C_Mem_Read(hi2c,BMP388_I2C_ADDR,reg,I2C_MEMADD_SIZE_8BIT,data,len,100) != HAL_OK)
					return BMP388_ERROR_I2C;
    
	return BMP388_OK;
}

static BMP388_StatusTypeDef BMP388_WriteRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t value)
{
	 if (HAL_I2C_Mem_Write(hi2c,BMP388_I2C_ADDR,reg,I2C_MEMADD_SIZE_8BIT,&value, 1,100) != HAL_OK)
					return BMP388_ERROR_I2C;
			

	return BMP388_OK;
}
	
static BMP388_StatusTypeDef BMP388_Config(I2C_HandleTypeDef *hi2c)
{
    /*
     * Configure OSR, ODR, IIR filter, and interrupt first.
     * Then enable pressure, temperature, and normal mode at the end.
     */

    if (BMP388_WriteRegister(hi2c, BMP388_REG_OSR, BMP388_OSR_CONFIG) != BMP388_OK)
    {
        return BMP388_ERROR_CONFIG;
    }

    if (BMP388_WriteRegister(hi2c, BMP388_REG_ODR, BMP388_ODR_CONFIG) != BMP388_OK)
    {
        return BMP388_ERROR_CONFIG;
    }

    if (BMP388_WriteRegister(hi2c, BMP388_REG_CONFIG, BMP388_IIR_CONFIG) != BMP388_OK)
    {
        return BMP388_ERROR_CONFIG;
    }

    if (BMP388_WriteRegister(hi2c, BMP388_REG_INT_CTRL, BMP388_INT_CTRL_CONFIG) != BMP388_OK)
    {
        return BMP388_ERROR_CONFIG;
    }
		

    return BMP388_OK;
}
/* ================= CALIBRATION ================= */

static BMP388_StatusTypeDef BMP388_ReadCalibration(I2C_HandleTypeDef *hi2c)
{
    uint8_t calib[BMP388_CALIB_DATA_LEN];
    BMP388_RawCalibTypeDef raw;

    if (BMP388_ReadRegister(hi2c, BMP388_REG_CALIB_DATA, calib, BMP388_CALIB_DATA_LEN) != BMP388_OK)
        return BMP388_ERROR_CALIBRATION;
    

    /*
     * Convert calibration bytes into raw calibration variables
     */
    raw.par_t1 = (uint16_t)((uint16_t)calib[0]  | ((uint16_t)calib[1]  << 8));
    raw.par_t2 = (uint16_t)((uint16_t)calib[2]  | ((uint16_t)calib[3]  << 8));
    raw.par_t3 = (int8_t)calib[4];

    raw.par_p1 = (int16_t)((uint16_t)calib[5]  | ((uint16_t)calib[6]  << 8));
    raw.par_p2 = (int16_t)((uint16_t)calib[7]  | ((uint16_t)calib[8]  << 8));
    raw.par_p3 = (int8_t)calib[9];
    raw.par_p4 = (int8_t)calib[10];

    raw.par_p5 = (uint16_t)((uint16_t)calib[11] | ((uint16_t)calib[12] << 8));
    raw.par_p6 = (uint16_t)((uint16_t)calib[13] | ((uint16_t)calib[14] << 8));
    raw.par_p7 = (int8_t)calib[15];
    raw.par_p8 = (int8_t)calib[16];

    raw.par_p9  = (int16_t)((uint16_t)calib[17] | ((uint16_t)calib[18] << 8));
    raw.par_p10 = (int8_t)calib[19];
    raw.par_p11 = (int8_t)calib[20];

    /*
     * Convert raw calibration variables into floating-point coefficients
     */
    bmp388_calib.par_t1 = (float)raw.par_t1 / 0.00390625f;
    bmp388_calib.par_t2 = (float)raw.par_t2 / 1073741824.0f;
    bmp388_calib.par_t3 = (float)raw.par_t3 / 281474976710656.0f;

    bmp388_calib.par_p1  = ((float)raw.par_p1 - 16384.0f) / 1048576.0f;
    bmp388_calib.par_p2  = ((float)raw.par_p2 - 16384.0f) / 536870912.0f;
    bmp388_calib.par_p3  = (float)raw.par_p3 / 4294967296.0f;
    bmp388_calib.par_p4  = (float)raw.par_p4 / 137438953472.0f;
    bmp388_calib.par_p5  = (float)raw.par_p5 / 0.125f;
    bmp388_calib.par_p6  = (float)raw.par_p6 / 64.0f;
    bmp388_calib.par_p7  = (float)raw.par_p7 / 256.0f;
    bmp388_calib.par_p8  = (float)raw.par_p8 / 32768.0f;
    bmp388_calib.par_p9  = (float)raw.par_p9 / 281474976710656.0f;
    bmp388_calib.par_p10 = (float)raw.par_p10 / 281474976710656.0f;
    bmp388_calib.par_p11 = (float)raw.par_p11 / 36893488147419103232.0f;

    bmp388_calib.t_lin = 0.0f;

    return BMP388_OK;
}



/* ================= INIT ================= */


BMP388_StatusTypeDef BMP388_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t chip_id = 0;

    if (hi2c == NULL)
    {
        return BMP388_ERROR_NULL_PTR;
    }

    /* 1. Read and check chip ID */
    if (BMP388_ReadRegister(hi2c, BMP388_REG_CHIP_ID, &chip_id, 1) != BMP388_OK)
    {
        return BMP388_ERROR_I2C;
    }

    if (chip_id != BMP388_CHIP_ID_VALUE)
    {
        return BMP388_ERROR_CHIP_ID;
    }

    /* 2. Soft reset */
    if (BMP388_WriteRegister(hi2c, BMP388_REG_CMD, BMP388_SOFT_RESET_CMD) != BMP388_OK)
    {
        return BMP388_ERROR_RESET;
    }

    HAL_Delay(10);

    /* 3. Read calibration data */
    if (BMP388_ReadCalibration(hi2c) != BMP388_OK)
    {
        return BMP388_ERROR_CALIBRATION;
    }

    /* 4. Configure OSR, ODR, IIR filter, and interrupt */
    if (BMP388_Config(hi2c) != BMP388_OK)
    {
        return BMP388_ERROR_CONFIG;
    }

    /* 5. Enable pressure + temperature + normal mode */
    if (BMP388_WriteRegister(hi2c, BMP388_REG_PWR_CTRL, BMP388_PWR_CTRL_CONFIG) != BMP388_OK)
    {
        return BMP388_ERROR_CONFIG;
    }

    return BMP388_OK;
}



/* ================= RAW DATA ================= */

/* ================= RAW DATA ================= */

static BMP388_StatusTypeDef BMP388_ReadRaw(I2C_HandleTypeDef *hi2c, uint32_t *raw_press,uint32_t *raw_temp)
{
    uint8_t data[6];

    if (BMP388_ReadRegister(hi2c, BMP388_REG_PRESS_DATA_START, data, 6) != BMP388_OK)
    {
        return BMP388_ERROR_RAW_DATA;
    }

    /*
     * data[0] = pressure XLSB
     * data[1] = pressure LSB
     * data[2] = pressure MSB
     *
     * data[3] = temperature XLSB
     * data[4] = temperature LSB
     * data[5] = temperature MSB
     */
    *raw_press = ((uint32_t)data[2] << 16) |
                 ((uint32_t)data[1] << 8)  |
                 ((uint32_t)data[0]);

    *raw_temp = ((uint32_t)data[5] << 16) |
                ((uint32_t)data[4] << 8)  |
                ((uint32_t)data[3]);

    return BMP388_OK;
}

/* ================= COMPENSATION ================= */

/* ================= COMPENSATION ================= */

static float BMP388_CompensateTemperature(uint32_t adc_temp)
{
    float partial_data1;
    float partial_data2;

    partial_data1 = (float)adc_temp - bmp388_calib.par_t1;
    partial_data2 = partial_data1 * bmp388_calib.par_t2;

    bmp388_calib.t_lin = partial_data2 +
                         (partial_data1 * partial_data1) * bmp388_calib.par_t3;

    return bmp388_calib.t_lin;
}

static float BMP388_CompensatePressure(uint32_t uncomp_press)
{
    float partial_data1, partial_data2, partial_data3;
    float partial_data4, partial_data5;
    float pressure;

    partial_data1 = bmp388_calib.par_p6 * bmp388_calib.t_lin;
    partial_data2 = bmp388_calib.par_p7 * (bmp388_calib.t_lin * bmp388_calib.t_lin);
    partial_data3 = bmp388_calib.par_p8 * (bmp388_calib.t_lin * bmp388_calib.t_lin * bmp388_calib.t_lin);
    partial_data4 = bmp388_calib.par_p5 + partial_data1 + partial_data2 + partial_data3;

    partial_data1 = bmp388_calib.par_p2 * bmp388_calib.t_lin;
    partial_data2 = bmp388_calib.par_p3 * (bmp388_calib.t_lin * bmp388_calib.t_lin);
    partial_data3 = bmp388_calib.par_p4 * (bmp388_calib.t_lin * bmp388_calib.t_lin * bmp388_calib.t_lin);
    partial_data5 = (float)uncomp_press * (bmp388_calib.par_p1 + partial_data1 + partial_data2 + partial_data3);

    partial_data1 = (float)uncomp_press * (float)uncomp_press;
    partial_data2 = bmp388_calib.par_p9 + bmp388_calib.par_p10 * bmp388_calib.t_lin;
    partial_data3 = partial_data1 * partial_data2;

    partial_data1 = partial_data1 * (float)uncomp_press;
    partial_data2 = partial_data1 * bmp388_calib.par_p11;

    pressure = partial_data4 + partial_data5 + partial_data3 + partial_data2;

    return pressure;
}

/* altitiud function*/
static float BMP388_CalculateAltitude(float pressure_pa)
{
    if (bmp388_reference_pressure == 0.0f)
    {
        bmp388_reference_pressure = pressure_pa;
        return 0.0f;
    }

    return 44330.0f *
           (1.0f - powf(pressure_pa / bmp388_reference_pressure, 0.1903f));
}

/* ================= PUBLIC ================= */

BMP388_StatusTypeDef BMP388_GetData(I2C_HandleTypeDef *hi2c,float *temperature,float *pressure,float *altitude)
{
      uint32_t raw_temp;
    uint32_t raw_press;

    if (BMP388_ReadRaw(hi2c, &raw_press, &raw_temp) != BMP388_OK)
    {
        return BMP388_ERROR_RAW_DATA;
    }

    /*
     * Temperature must be compensated first.
     * Pressure compensation uses bmp388_calib.t_lin.
     */
    *temperature = BMP388_CompensateTemperature(raw_temp);
    *pressure    = BMP388_CompensatePressure(raw_press);

    /*
     * Altitude is calculated from pressure in Pa.
     */
    *altitude = BMP388_CalculateAltitude(*pressure);

    return BMP388_OK;
}

