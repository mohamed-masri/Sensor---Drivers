#ifndef RA01_REGISTERS_H
#define RA01_REGISTERS_H

#include <stdint.h>

/* SPI operation control */
#define RA01_SPI_WRITE_BIT       0x80U
#define RA01_SPI_READ_MASK    	 0x7FU

/* SX1278 registers */
#define RA01_REG_VERSION        				 0x42U
#define RA01_REG_OP_MODE                 0x01U

/* RegOpMode bit definitions */
#define RA01_LONG_RANGE_MODE             0x80U
#define RA01_ACCESS_SHARED_REG           0x40U
#define RA01_LOW_FREQUENCY_MODE          0x08U

/* Bits 2:0 select the radio operating mode. */
#define RA01_OPMODE_MODE_MASK            0x07U

/* Carrier-frequency registers */
#define RA01_REG_FRF_MSB              0x06U
#define RA01_REG_FRF_MID              0x07U
#define RA01_REG_FRF_LSB              0x08U

/* SX1278 frequency-synthesizer constants */
#define RA01_XOSC_FREQUENCY_HZ        32000000UL
#define RA01_FRF_SHIFT                19U

/* FIFO data register */
#define RA01_REG_FIFO                    0x00U

/* FIFO control registers */
#define RA01_REG_FIFO_ADDR_PTR           0x0DU
#define RA01_REG_FIFO_TX_BASE_ADDR       0x0EU
#define RA01_REG_FIFO_RX_BASE_ADDR       0x0FU

/* RX packet information registers */
#define RA01_REG_FIFO_RX_CURRENT_ADDR    0x10U
#define RA01_REG_RX_NB_BYTES             0x13U

/* Default FIFO-memory starting addresses */
#define RA01_DEFAULT_RX_FIFO_BASE        0x00U
#define RA01_DEFAULT_TX_FIFO_BASE        0x80U

/* LoRa modem configuration registers */
#define RA01_REG_MODEM_CONFIG_1          0x1DU

/* RegModemConfig1 field masks */
#define RA01_BANDWIDTH_MASK              0xF0U
#define RA01_CODING_RATE_MASK            0x0EU
#define RA01_HEADER_MODE_MASK            0x01U

/* LoRa modem configuration register 2 */
#define RA01_REG_MODEM_CONFIG_2          0x1EU

/* RegModemConfig2 field masks */
#define RA01_SPREADING_FACTOR_MASK       0xF0U
#define RA01_TX_CONTINUOUS_MODE_BIT      0x08U
#define RA01_PAYLOAD_CRC_MASK            0x04U
#define RA01_SYMBOL_TIMEOUT_MSB_MASK     0x03U

/* LoRa modem configuration register 3 */
#define RA01_REG_MODEM_CONFIG_3              0x26U

/* RegModemConfig3 control bits */
#define RA01_LOW_DATA_RATE_OPTIMIZE_BIT      0x08U
#define RA01_AGC_AUTO_ON_BIT                 0x04U

/* LoRa preamble-length registers */
#define RA01_REG_PREAMBLE_MSB          0x20U
#define RA01_REG_PREAMBLE_LSB          0x21U

/* LoRa network sync-word register */
#define RA01_REG_SYNC_WORD              0x39U

/* Power-amplifier configuration register */
#define RA01_REG_PA_CONFIG              0x09U

/* RegPaConfig fields */
#define RA01_PA_SELECT_BOOST            0x80U
#define RA01_OUTPUT_POWER_MASK          0x0FU

/* IRQ registers */
#define RA01_REG_IRQ_FLAGS_MASK          0x11U
#define RA01_REG_IRQ_FLAGS               0x12U

/* RegIrqFlags bits */
#define RA01_IRQ_RX_TIMEOUT              0x80U
#define RA01_IRQ_RX_DONE                 0x40U
#define RA01_IRQ_PAYLOAD_CRC_ERROR       0x20U
#define RA01_IRQ_VALID_HEADER            0x10U
#define RA01_IRQ_TX_DONE                 0x08U
#define RA01_IRQ_CAD_DONE                0x04U
#define RA01_IRQ_FHSS_CHANGE_CHANNEL     0x02U
#define RA01_IRQ_CAD_DETECTED            0x01U

#define RA01_IRQ_ALL                     0xFFU

/* DIO mapping register */
#define RA01_REG_DIO_MAPPING_1          0x40U

/* RegDioMapping1 bits 7:6 control DIO0 */
#define RA01_DIO0_MAPPING_MASK          0xC0U

/* Payload length register */
#define RA01_REG_PAYLOAD_LENGTH          0x22U





#endif 
