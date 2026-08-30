#include "ina219.h"

/* INA219 registers */
#define INA219_REG_CONFIG          0x00U
#define INA219_REG_SHUNT_VOLTAGE   0x01U
#define INA219_REG_BUS_VOLTAGE     0x02U

/* 32 V range, ±320 mV shunt range, 12-bit continuous mode */
#define INA219_CONFIG_VALUE        0x399FU

#define INA219_TIMEOUT_MS          100U

static HAL_StatusTypeDef INA219_WriteRegister(uint8_t reg,
                                               uint16_t value)
{
    uint8_t data[2];

    data[0] = (uint8_t)(value >> 8);    // Send MSB first
    data[1] = (uint8_t)(value & 0xFFU); // Send LSB second

    return HAL_I2C_Mem_Write(
        INA219_I2C_HANDLE,
        INA219_I2C_ADDRESS,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        data,
        2U,
        INA219_TIMEOUT_MS
    );
}

static HAL_StatusTypeDef INA219_ReadRegister(uint8_t reg,
                                              uint16_t *value)
{
    uint8_t data[2];
    HAL_StatusTypeDef status;

    if (value == NULL)
    {
        return HAL_ERROR;
    }

    status = HAL_I2C_Mem_Read(
        INA219_I2C_HANDLE,
        INA219_I2C_ADDRESS,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        data,
        2U,
        INA219_TIMEOUT_MS
    );

    if (status != HAL_OK)
    {
        return status;
    }

    *value = ((uint16_t)data[0] << 8) | data[1];

    return HAL_OK;
}

HAL_StatusTypeDef INA219_Init(void)
{
    HAL_StatusTypeDef status;

    /* Check whether INA219 responds */
    status = HAL_I2C_IsDeviceReady(
        INA219_I2C_HANDLE,
        INA219_I2C_ADDRESS,
        3U,
        INA219_TIMEOUT_MS
    );

    if (status != HAL_OK)
    {
        return status;
    }

    /* Configure continuous bus and shunt measurement */
    status = INA219_WriteRegister(
        INA219_REG_CONFIG,
        INA219_CONFIG_VALUE
    );

    if (status != HAL_OK)
    {
        return status;
    }

    HAL_Delay(10U); // Wait for the first conversion

    return HAL_OK;
}

HAL_StatusTypeDef INA219_Read(INA219_Data_t *data)
{
    uint16_t bus_raw;
    uint16_t shunt_register;
    int16_t shunt_raw;
    HAL_StatusTypeDef status;

    if (data == NULL)
    {
        return HAL_ERROR;
    }

    /* Read bus voltage at VIN- */
    status = INA219_ReadRegister(
        INA219_REG_BUS_VOLTAGE,
        &bus_raw
    );

    if (status != HAL_OK)
    {
        return status;
    }

    /* Read voltage across VIN+ and VIN- */
    status = INA219_ReadRegister(
        INA219_REG_SHUNT_VOLTAGE,
        &shunt_register
    );

    if (status != HAL_OK)
    {
        return status;
    }

    /* Shunt register is a signed 16-bit value */
    shunt_raw = (int16_t)shunt_register;

    /* Bus register bits 15:3 use 4 mV per bit */
    data->bus_voltage_mV =
        (int32_t)(bus_raw >> 3) * 4L;

    /* Shunt voltage uses 10 microvolts per bit */
    data->shunt_voltage_uV =
        (int32_t)shunt_raw * 10L;

    /* I(mA) = Vshunt(uV) / Rshunt(milliohms) */
    data->current_mA =
        data->shunt_voltage_uV /
        INA219_SHUNT_MILLIOHMS;

    /* VIN+ voltage = VIN- voltage + shunt drop */
    data->battery_voltage_mV =
        data->bus_voltage_mV +
        (data->shunt_voltage_uV / 1000L);

    /* P(mW) = V(mV) × I(mA) / 1000 */
    data->power_mW =
        (int32_t)(
            ((int64_t)data->battery_voltage_mV *
             data->current_mA) / 1000LL
        );

    return HAL_OK;
}