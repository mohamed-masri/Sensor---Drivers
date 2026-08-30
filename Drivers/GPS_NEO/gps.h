#ifndef GPS_H
#define GPS_H



#include "stm32f7xx_hal.h"

#include <stdint.h>
#include <stdbool.h>


/*
 * GPS driver return status.
 */
typedef enum
{
    GPS_STATUS_OK = 0,
    GPS_STATUS_ERROR

} GPS_Status_t;


/*
 * GPS parameters required by the telemetry packet.
 *
 * Latitude and longitude:
 * Decimal degrees
 *
 * Altitude:
 * Metres above mean sea level
 */
typedef struct
{
    float latitude_deg;
    float longitude_deg;
    float altitude_m;

    bool fix_valid;
    bool new_data;

} GPS_Data_t;


/*
 * Initializes the GPS driver and starts UART DMA reception.
 *
 * Example:
 * GPS_Init(&huart3);
 */
GPS_Status_t GPS_Init(UART_HandleTypeDef *huart);


/*
 * Must be called from HAL_UARTEx_RxEventCallback().
 *
 * It copies the received DMA bytes into the GPS
 * software ring buffer and restarts DMA reception.
 */
void GPS_UART_RxEventCallback(UART_HandleTypeDef *huart,
                              uint16_t received_size);


/*
 * Processes the received GPS bytes.
 *
 * This function must be called repeatedly
 * inside the main loop.
 */
void GPS_Process(void);


/*
 * Parses one complete NMEA sentence.
 *
 * It accepts GGA sentences, including:
 *
 * $GNGGA
 * $GPGGA
 *
 * Returns:
 *
 * true  -> Valid GGA sentence was parsed.
 * false -> Unsupported, damaged or invalid sentence.
 */
bool GPS_ParseSentence(const char *sentence);


/*
 * Returns a read-only pointer to the latest GPS data.
 */
const GPS_Data_t *GPS_GetData(void);


/*
 * Returns true when the GPS has a valid position fix.
 */
bool GPS_IsFixValid(void);


/*
 * Returns true when new valid GPS data is available.
 */
bool GPS_IsNewDataAvailable(void);


/*
 * Clears the new-data flag after the application
 * has used the GPS values.
 */
void GPS_ClearNewDataFlag(void);


#endif /* GPS_H */