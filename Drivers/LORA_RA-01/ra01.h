#ifndef RA01_H
#define RA01_H

#include "stm32f7xx_hal.h"
#include <stdint.h>


/*==========================================================
 * User-changeable hardware configuration
 *==========================================================*/

/*
 * SPI peripheral used by the RA-01.
 */
extern SPI_HandleTypeDef hspi1;

#define RA01_SPI_HANDLE              (&hspi1)

/*
 * NSS pin configuration.
 */
#define RA01_NSS_GPIO_PORT           GPIOC
#define RA01_NSS_PIN                 GPIO_PIN_8

/*
 * RESET pin configuration.
 */
#define RA01_RESET_GPIO_PORT         GPIOC
#define RA01_RESET_PIN               GPIO_PIN_5

/*
 * DIO0 pin configuration
 *
 * Configure this pin in CubeMX as:
 * GPIO_EXTI4
 * Rising edge trigger
 * No pull-up / no pull-down
 */
#define RA01_DIO0_GPIO_PORT        GPIOA
#define RA01_DIO0_PIN              GPIO_PIN_4


/* Temporary operating frequency */
#define RA01_DEFAULT_FREQUENCY_HZ     433500000UL

/*
 * HAL SPI timeout.
 *
 * This timeout does not control the SPI clock speed.
 * SPI speed is configured in CubeMX.
 */
#define RA01_SPI_TIMEOUT_MS          10

/* Default LoRa preamble length in symbols. */
#define RA01_DEFAULT_PREAMBLE_LENGTH   8U

/* Initial private-network sync word. */
#define RA01_DEFAULT_SYNC_WORD          0x2DU

/* Provisional transmit power. */
#define RA01_DEFAULT_TX_POWER_DBM       13


#define RA01_EXPECTED_VERSION    0x12U

/* SX1278 reference oscillator */
#define RA01_XOSC_FREQUENCY_HZ        32000000UL
#define RA01_FRF_SHIFT                19U // ==  2^19

/* Internal received-packet buffer size. */
#define RA01_RX_BUFFER_SIZE        30U


/*==========================================================
 * Driver status
 *==========================================================*/

typedef enum
{
    RA01_OK = 0,
    RA01_ERROR,
    RA01_SPI_ERROR,
    RA01_INVALID_PARAMETER,
    RA01_VERSION_MISMATCH,
    RA01_NO_PACKET,
    RA01_CRC_ERROR,
    RA01_BUFFER_TOO_SMALL,
    RA01_BUSY

} RA01_Status_t;

/*==========================================================*/
/* Radio operating modes available to application code. */
typedef enum
{
    RA01_MODE_SLEEP         = 0x00U,
    RA01_MODE_STANDBY       = 0x01,
    RA01_MODE_FSTX          = 0x02U,
    RA01_MODE_TX            = 0x03U,
    RA01_MODE_FSRX          = 0x04U,
    RA01_MODE_RX_CONTINUOUS = 0x05U,
    RA01_MODE_RX_SINGLE     = 0x06U,
    RA01_MODE_CAD           = 0x07U

} RA01_Mode_t;

/* LoRa signal bandwidth values for RegModemConfig1 bits 7:4. */
typedef enum
{
    RA01_BW_7_8_KHZ   = 0x00U,
    RA01_BW_10_4_KHZ  = 0x10U,
    RA01_BW_15_6_KHZ  = 0x20U,
    RA01_BW_20_8_KHZ  = 0x30U,
    RA01_BW_31_25_KHZ = 0x40U,
    RA01_BW_41_7_KHZ  = 0x50U,
    RA01_BW_62_5_KHZ  = 0x60U,
    RA01_BW_125_KHZ   = 0x70U,
    RA01_BW_250_KHZ   = 0x80U,
    RA01_BW_500_KHZ   = 0x90U

} RA01_Bandwidth_t;


/* LoRa error-correction coding-rate values for bits 3:1. */
typedef enum
{
    RA01_CR_4_5 = 0x02U,
    RA01_CR_4_6 = 0x04U,
    RA01_CR_4_7 = 0x06U,
    RA01_CR_4_8 = 0x08U

} RA01_CodingRate_t;


/* Packet-header mode value for bit 0. */
typedef enum
{
    RA01_HEADER_EXPLICIT = 0x00U,
    RA01_HEADER_IMPLICIT = 0x01U

} RA01_HeaderMode_t;


/* LoRa spreading-factor values for RegModemConfig2 bits 7:4. */
typedef enum
{
    RA01_SF_6  = 0x60U,
    RA01_SF_7  = 0x70U,
    RA01_SF_8  = 0x80U,
    RA01_SF_9  = 0x90U,
    RA01_SF_10 = 0xA0U,
    RA01_SF_11 = 0xB0U,
    RA01_SF_12 = 0xC0U

} RA01_SpreadingFactor_t;


/* Payload CRC values for RegModemConfig2 bit 2. */
typedef enum
{
    RA01_CRC_DISABLED = 0x00U,
    RA01_CRC_ENABLED  = 0x04U

} RA01_CrcMode_t;

/* Low-data-rate optimization setting. */
typedef enum
{
    RA01_LDRO_DISABLED = 0x00U,
    RA01_LDRO_ENABLED  = 0x08U

} RA01_LdroMode_t;


/* Automatic receiver-gain-control setting. */
typedef enum
{
    RA01_AGC_DISABLED = 0x00U,
    RA01_AGC_ENABLED  = 0x04U

} RA01_AgcMode_t;

/* DIO0 interrupt source selection. */
typedef enum
{
    RA01_DIO0_MAP_RX_DONE  = 0x00U,
    RA01_DIO0_MAP_TX_DONE  = 0x40U,
    RA01_DIO0_MAP_CAD_DONE = 0x80U

} RA01_Dio0Mapping_t;




/*==========================================================
 * Public function declarations
 *==========================================================*/

/**
 Reset the SX1278 using the RESET pin.
 */
void RA01_Reset(void);

/**
 *  Write one byte to an SX1278 register.
 *  address Register address.
 *  value   Value written to the register.
 */
RA01_Status_t RA01_WriteRegister(uint8_t address, uint8_t value);

/**
   Read one byte from an SX1278 register.
 *  address Register address.
 *  value   Pointer where the register value will be stored.
 */
RA01_Status_t RA01_ReadRegister(uint8_t address,  uint8_t *value);


 
 /*
 RA01_OK if RegVersion equals 0x12.
 */
RA01_Status_t RA01_CheckConnection(void);


/* Change the operating-mode field in RegOpMode. */
RA01_Status_t RA01_SetMode(RA01_Mode_t mode);

/* Enable LoRa mode and select the low-frequency path. */
RA01_Status_t RA01_EnableLoRaMode(void);

/* Configure the SX1278 carrier frequency. */
RA01_Status_t RA01_SetFrequency(uint32_t frequency_hz);

/* Configure the starting addresses of the RX and TX FIFO regions. */
RA01_Status_t RA01_SetFifoBaseAddresses(uint8_t rx_base,
                                        uint8_t tx_base);
																				

/* Configure bandwidth, coding rate, and packet-header mode. */
RA01_Status_t RA01_SetModemConfig1(
    RA01_Bandwidth_t bandwidth,
    RA01_CodingRate_t coding_rate,
    RA01_HeaderMode_t header_mode
);

/* Configure the spreading factor and payload CRC. */
RA01_Status_t RA01_SetModemConfig2(
    RA01_SpreadingFactor_t spreading_factor,
    RA01_CrcMode_t crc_mode
);

/* Configure low-data-rate optimization and automatic gain control. */
RA01_Status_t RA01_SetModemConfig3(
    RA01_LdroMode_t ldro_mode,
    RA01_AgcMode_t agc_mode
);

/* Configure the LoRa preamble length. */
RA01_Status_t RA01_SetPreambleLength(uint16_t preamble_length);

/* Configure the LoRa network sync word. */
RA01_Status_t RA01_SetSyncWord(uint8_t sync_word);

/* Configure PA_BOOST output power from +2 to +17 dBm. */
RA01_Status_t RA01_SetTxPower(int8_t power_dbm);

/* Clear selected SX1278 IRQ flags. */
RA01_Status_t RA01_ClearIrqFlags(uint8_t flags);

/* Map DIO0 to RxDone, TxDone, or CadDone. */
RA01_Status_t RA01_SetDio0Mapping(RA01_Dio0Mapping_t mapping);

/* Start continuous LoRa reception. */
RA01_Status_t RA01_StartRxContinuous(void);

/* Called by the STM32 EXTI callback when DIO0 rises. */
void RA01_Dio0IrqCallback(void);

/* initialazation function. */
RA01_Status_t RA01_Init(void);

/* Processes pending RA-01 events outside interrupt context. */
RA01_Status_t RA01_Process(void);

/* Read the last received packet from the SX1278 FIFO. */
RA01_Status_t RA01_ReadReceivedPacket(uint8_t *buffer,uint8_t buffer_size,uint8_t *received_length);

RA01_Status_t RA01_GetReceivedPacket(uint8_t *buffer,uint8_t buffer_size,uint8_t *received_length);

/* Start non-blocking LoRa transmission. */
RA01_Status_t RA01_TransmitAsync(const uint8_t *data, uint8_t length);
#endif 