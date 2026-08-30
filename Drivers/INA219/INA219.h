#ifndef INA219_H
#define INA219_H

#include "stm32f7xx_hal.h"


/* I2C peripheral */
extern I2C_HandleTypeDef hi2c1;

#define INA219_I2C_HANDLE          (&hi2c1)

/* HAL uses the shifted I2C address */
#define INA219_I2C_ADDRESS         (0x40U << 1)

/* R100 means 0.1 ohm = 100 milliohms */
#define INA219_SHUNT_MILLIOHMS     100L

typedef struct
{
    int32_t bus_voltage_mV;
    int32_t shunt_voltage_uV;
    int32_t battery_voltage_mV;
    int32_t current_mA;
    int32_t power_mW;

} INA219_Data_t;

HAL_StatusTypeDef INA219_Init(void);

HAL_StatusTypeDef INA219_Read(INA219_Data_t *data);

#endif