#ifndef BMP388_H_
#define BMP388_H_

#include "stm32f7xx_hal.h"
#include <stdint.h>


/* ============================================================
 * BMP388 I2C ADDRESS
 * ============================================================ */

#define BMP388_I2C_ADDR        (0x76 << 1)   // SDO pin connected to GND


/* ============================================================
 * BMP388 REGISTER MAP
 * ============================================================ */

#define BMP388_REG_CHIP_ID     0x00    // Chip ID register
#define BMP388_REG_ERR         0x02    // Error register
#define BMP388_REG_STATUS      0x03    // Status register

#define BMP388_REG_PRESS_DATA_START       0x04    // Start of pressure/temp data registers

#define BMP388_REG_INT_STATUS  0x11    // Interrupt status register
#define BMP388_REG_INT_CTRL    0x19    // Interrupt control register
#define BMP388_REG_PWR_CTRL    0x1B    // Power control register
#define BMP388_REG_OSR         0x1C    // Oversampling register
#define BMP388_REG_ODR         0x1D    // Output data rate register
#define BMP388_REG_CONFIG      0x1F    // IIR filter config register

#define BMP388_REG_CALIB_DATA  0x31    // Start of calibration data
#define BMP388_REG_CMD         0x7E    // Command register


/* ============================================================
 * BMP388 FIXED VALUES
 * ============================================================ */

#define BMP388_CHIP_ID_VALUE   0x50    // Expected chip ID
#define BMP388_SOFT_RESET_CMD  0xB6    // Soft reset command

#define BMP388_RAW_DATA_LEN    6       // 3 pressure bytes + 3 temperature bytes
#define BMP388_CALIB_DATA_LEN  21      // Calibration data length

/* altitude parameters*/ 
#define BMP388_SEA_LEVEL_PRESSURE_PA    101325.0f  // sea level pressure 

/* ============================================================
 * BMP388 CONFIGURATION VALUES
 * ============================================================ */

/*
 * INT_CTRL register = 0x42
 *
 * push-pull output
 * active high
 * non-latched interrupt
 * data-ready interrupt enabled
 *
 * STM32 EXTI should be configured as rising edge.
 */
#define BMP388_INT_CTRL_CONFIG    0x42


/*
 * PWR_CTRL register = 0x33
 *
 * pressure enabled
 * temperature enabled
 * normal mode
 */
#define BMP388_PWR_CTRL_CONFIG    0x33

/*
 * OSR register = 0x03
 *
 * Drone table:
 * pressure oversampling    x8
 * temperature oversampling x1
 *
 * osrs_p = 011 -> x8
 * osrs_t = 000 -> x1
 */

#define BMP388_OSR_CONFIG         0x03

/*
 * ODR register = 0x02
 *
 * Drone table:
 * output data rate = 50 Hz
 */
#define BMP388_ODR_CONFIG         0x02


/*
 * CONFIG register = 0x04
 *
 * Drone table:
 * IIR filter setting = 2
 *
 * CONFIG bits [3:1] = 010
 * Register value = 010 << 1 = 0x04
 *
 * Note:
 * In the CONFIG register, iir_filter = 010 corresponds
 * to coef_3 internally.
 */
#define BMP388_IIR_CONFIG         0x04


/* calibration struct*/
typedef struct
{
    uint16_t par_t1;
    uint16_t par_t2;
    int8_t   par_t3;

    int16_t  par_p1;
    int16_t  par_p2;
    int8_t   par_p3;
    int8_t   par_p4;
    uint16_t par_p5;
    uint16_t par_p6;
    int8_t   par_p7;
    int8_t   par_p8;
    int16_t  par_p9;
    int8_t   par_p10;
    int8_t   par_p11;

} BMP388_RawCalibTypeDef;

typedef struct
{
    float par_t1;
    float par_t2;
    float par_t3;

    float par_p1;
    float par_p2;
    float par_p3;
    float par_p4;
    float par_p5;
    float par_p6;
    float par_p7;
    float par_p8;
    float par_p9;
    float par_p10;
    float par_p11;

    float t_lin;

} BMP388_CalibTypeDef;
/* ============================================================
 * DRIVER STATUS
 * ============================================================ */

typedef enum
{
    BMP388_OK = 0,

    BMP388_ERROR_I2C,          // Low-level I2C communication failed
    BMP388_ERROR_NULL_PTR,     // NULL pointer passed to function

    BMP388_ERROR_CHIP_ID,      // Wrong chip ID
    BMP388_ERROR_RESET,        // Soft reset failed
    BMP388_ERROR_CONFIG,       // Configuration failed
    BMP388_ERROR_CALIBRATION,  // Calibration data read failed
    BMP388_ERROR_RAW_DATA,     // Raw pressure/temp data read failed
    BMP388_ERROR_INIT,          // General init error, used only if needed
		BMP388_ERROR_GET_DATA

} BMP388_StatusTypeDef;


/* Public functions */

BMP388_StatusTypeDef BMP388_Init(I2C_HandleTypeDef *hi2c);
BMP388_StatusTypeDef BMP388_GetData(I2C_HandleTypeDef *hi2c,float *temperature,float *pressure,float *altitude);

#endif /* BMP388_H_ */