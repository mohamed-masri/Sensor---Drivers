#include "ra01.h"
#include "ra01_registers.h"

/* Internal radio state used to interpret DIO0 events. */
typedef enum
{
    RA01_STATE_STANDBY = 0,
    RA01_STATE_RX_CONTINUOUS,
    RA01_STATE_TX

} RA01_State_t;

/* DIO0 EXTI sets this flag; SPI processing is done later. */
static volatile uint8_t ra01_dio0_irq_pending = 0U;

/* Current driver state. */
static RA01_State_t ra01_state = RA01_STATE_STANDBY;

static uint8_t ra01_rx_buffer[RA01_RX_BUFFER_SIZE];
static uint8_t ra01_rx_length = 0U;
static uint8_t ra01_rx_packet_available = 0U;



/* Select the SX1278 by pulling NSS low. */
static void RA01_Select(void)
{
    RA01_NSS_GPIO_PORT->BSRR =
        ((uint32_t)RA01_NSS_PIN << 16U);
}


/* Release the SX1278 by pulling NSS high. */
static void RA01_Unselect(void)
{
    RA01_NSS_GPIO_PORT->BSRR =
        RA01_NSS_PIN;
}


/* Perform a hardware reset and wait until the SX1278 is ready. */
void RA01_Reset(void)
{
    /* RESET low */
    RA01_RESET_GPIO_PORT->BSRR =
        ((uint32_t)RA01_RESET_PIN << 16U);

    HAL_Delay(1);

    /* RESET high */
    RA01_RESET_GPIO_PORT->BSRR =
        RA01_RESET_PIN;

    HAL_Delay(10);
}


/* Write one byte to an SX1278 register. */
RA01_Status_t RA01_WriteRegister(uint8_t address, uint8_t value)
{
    HAL_StatusTypeDef hal_status;
    uint8_t tx_data[2];

    /* Bit 7 must be set for a write operation. */
    tx_data[0] = address | RA01_SPI_WRITE_BIT;
    tx_data[1] = value;

    RA01_Select();

    hal_status = HAL_SPI_Transmit(
        RA01_SPI_HANDLE,
        tx_data,
        2U,
        RA01_SPI_TIMEOUT_MS
    );

    RA01_Unselect();

    if (hal_status != HAL_OK)
    {
        return RA01_SPI_ERROR;
    }

    return RA01_OK;
}


/* Read one byte from an SX1278 register. */
RA01_Status_t RA01_ReadRegister(uint8_t address, uint8_t *value)
{
    HAL_StatusTypeDef hal_status;
    uint8_t tx_data[2];
    uint8_t rx_data[2];

    if (value == NULL)
    {
        return RA01_INVALID_PARAMETER;
    }

    /* Bit 7 must be cleared for a read operation. */
    tx_data[0] = address & RA01_SPI_READ_MASK;

    /* Dummy byte generates the clocks needed to receive data. */
    tx_data[1] = 0x00U;

    RA01_Select();

    hal_status = HAL_SPI_TransmitReceive(
        RA01_SPI_HANDLE,
        tx_data,
        rx_data,
        2U,
        RA01_SPI_TIMEOUT_MS
    );

    RA01_Unselect();

    if (hal_status != HAL_OK)
    {
        return RA01_SPI_ERROR;
    }

    /* The register value is received during the second byte. */
    *value = rx_data[1];

    return RA01_OK;
}


/* Read RegVersion and verify that the SX1278 responds with 0x12. */
RA01_Status_t RA01_CheckConnection()
{
    RA01_Status_t status;
    uint8_t received_version = 0;

    status = RA01_ReadRegister(
        RA01_REG_VERSION,
        &received_version
    );

    if (status != RA01_OK)
    {
        return status;
    }

    if (received_version != RA01_EXPECTED_VERSION)
    {
        return RA01_VERSION_MISMATCH;
    }

    return RA01_OK;
}

/*==========================================================*/
/* Change only bits 2:0 of RegOpMode. */
RA01_Status_t RA01_SetMode(RA01_Mode_t mode)
{
    RA01_Status_t status;
    uint8_t op_mode;

    if ((uint8_t)mode > RA01_MODE_CAD)
    {
        return RA01_INVALID_PARAMETER;
    }

    status = RA01_ReadRegister(RA01_REG_OP_MODE, &op_mode);

    if (status != RA01_OK)
    {
        return status;
    }

    /* Clear the current mode and insert the requested mode. */
    op_mode &= (uint8_t)(~RA01_OPMODE_MODE_MASK);
    op_mode |= ((uint8_t)mode & RA01_OPMODE_MODE_MASK);

    status = RA01_WriteRegister(RA01_REG_OP_MODE, op_mode);

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * Give the oscillator time to start when leaving Sleep mode.
     * This delay is mainly relevant for Sleep -> Standby/active mode.
     */
    if (mode != RA01_MODE_SLEEP)
    {
        HAL_Delay(1U);
    }

    return RA01_OK;
}

/* Enable LoRa mode at frequencies below 525 MHz. */
RA01_Status_t RA01_EnableLoRaMode(void)
{
    RA01_Status_t status;
    uint8_t op_mode;

    /* LongRangeMode can only be changed while sleeping. */
    status = RA01_SetMode(RA01_MODE_SLEEP);

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * LoRa mode enabled.
     * Shared FSK registers disabled.
     * Low-frequency path enabled.
     * Radio remains in Sleep mode.
     */
    op_mode = RA01_LONG_RANGE_MODE |
              RA01_LOW_FREQUENCY_MODE |
              RA01_MODE_SLEEP;

    return RA01_WriteRegister(RA01_REG_OP_MODE, op_mode);
}

/* Calculate and write the 24-bit carrier-frequency value. */
RA01_Status_t RA01_SetFrequency(uint32_t frequency_hz)
{
    RA01_Status_t status;
    uint32_t frf_value;

    /* SX1278 operating-frequency range. */
    if ((frequency_hz < 137000000UL) ||
        (frequency_hz > 525000000UL))
    {
        return RA01_INVALID_PARAMETER;
    }

    /*
     * FRF = frequency_hz × 2^19 / 32 MHz.
     *
     * The calculation is performed in 64 bits to prevent
     * overflow before the division.
     */
    frf_value = (uint32_t)(
        ((uint64_t)frequency_hz << RA01_FRF_SHIFT) /
        RA01_XOSC_FREQUENCY_HZ
    );

    /* Write the most-significant frequency byte. */
    status = RA01_WriteRegister(
        RA01_REG_FRF_MSB,
        (uint8_t)(frf_value >> 16U)
    );

    if (status != RA01_OK)
    {
        return status;
    }

    /* Write the middle frequency byte. */
    status = RA01_WriteRegister(
        RA01_REG_FRF_MID,
        (uint8_t)(frf_value >> 8U)
    );

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * Writing RegFrfLsb completes the frequency update.
     */
    return RA01_WriteRegister(
        RA01_REG_FRF_LSB,
        (uint8_t)frf_value
    );
}

/* Configure the starting addresses of the RX and TX FIFO regions. */
RA01_Status_t RA01_SetFifoBaseAddresses(uint8_t rx_base,
                                        uint8_t tx_base)
{
    RA01_Status_t status;

    /*
     * RX and TX must not start from the same FIFO address.
     */
    if (rx_base == tx_base)
    {
        return RA01_INVALID_PARAMETER;
    }

    status = RA01_WriteRegister(
        RA01_REG_FIFO_RX_BASE_ADDR,
        rx_base
    );

    if (status != RA01_OK)
    {
        return status;
    }

    return RA01_WriteRegister(
        RA01_REG_FIFO_TX_BASE_ADDR,
        tx_base
    );
}

/* Configure bandwidth, coding rate, and packet-header mode. */
RA01_Status_t RA01_SetModemConfig1(
    RA01_Bandwidth_t bandwidth,
    RA01_CodingRate_t coding_rate,
    RA01_HeaderMode_t header_mode)
{
    uint8_t modem_config_1;

    /* Reject values containing bits outside their register fields. */
    if ((((uint8_t)bandwidth & (uint8_t)~RA01_BANDWIDTH_MASK) != 0U) ||
        (((uint8_t)coding_rate & (uint8_t)~RA01_CODING_RATE_MASK) != 0U) ||
        (((uint8_t)header_mode & (uint8_t)~RA01_HEADER_MODE_MASK) != 0U))
    {
        return RA01_INVALID_PARAMETER;
    }

    /* Combine the three fields into the complete register value. */
    modem_config_1 = (uint8_t)bandwidth |
                     (uint8_t)coding_rate |
                     (uint8_t)header_mode;

    return RA01_WriteRegister(
        RA01_REG_MODEM_CONFIG_1,
        modem_config_1
    );
}



/* Configure the spreading factor and payload CRC. */
RA01_Status_t RA01_SetModemConfig2(
    RA01_SpreadingFactor_t spreading_factor,
    RA01_CrcMode_t crc_mode)
{
    RA01_Status_t status;
    uint8_t modem_config_2;

    /* Accept only SF6 through SF12. */
    if (((uint8_t)spreading_factor < (uint8_t)RA01_SF_6) ||
        ((uint8_t)spreading_factor > (uint8_t)RA01_SF_12))
    {
        return RA01_INVALID_PARAMETER;
    }

    if ((crc_mode != RA01_CRC_DISABLED) &&
        (crc_mode != RA01_CRC_ENABLED))
    {
        return RA01_INVALID_PARAMETER;
    }

    status = RA01_ReadRegister(
        RA01_REG_MODEM_CONFIG_2,
        &modem_config_2
    );

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * Clear the spreading-factor, TX-continuous and CRC fields.
     * Preserve the symbol-timeout bits 1:0.
     */
    modem_config_2 &= (uint8_t)~(
        RA01_SPREADING_FACTOR_MASK |
        RA01_TX_CONTINUOUS_MODE_BIT |
        RA01_PAYLOAD_CRC_MASK
    );

    modem_config_2 |= (uint8_t)spreading_factor;
    modem_config_2 |= (uint8_t)crc_mode;

    return RA01_WriteRegister(
        RA01_REG_MODEM_CONFIG_2,
        modem_config_2
    );
}


/* Configure low-data-rate optimization and automatic gain control. */
RA01_Status_t RA01_SetModemConfig3(
    RA01_LdroMode_t ldro_mode,
    RA01_AgcMode_t agc_mode)
{
    RA01_Status_t status;
    uint8_t modem_config_3;

    if ((ldro_mode != RA01_LDRO_DISABLED) &&
        (ldro_mode != RA01_LDRO_ENABLED))
    {
        return RA01_INVALID_PARAMETER;
    }

    if ((agc_mode != RA01_AGC_DISABLED) &&
        (agc_mode != RA01_AGC_ENABLED))
    {
        return RA01_INVALID_PARAMETER;
    }

    status = RA01_ReadRegister(
        RA01_REG_MODEM_CONFIG_3,
        &modem_config_3
    );

    if (status != RA01_OK)
    {
        return status;
    }

    /* Clear the old LDRO and AGC settings. */
    modem_config_3 &= (uint8_t)~(
        RA01_LOW_DATA_RATE_OPTIMIZE_BIT |
        RA01_AGC_AUTO_ON_BIT
    );

    /* Insert the requested settings. */
    modem_config_3 |= (uint8_t)ldro_mode;
    modem_config_3 |= (uint8_t)agc_mode;

    return RA01_WriteRegister(
        RA01_REG_MODEM_CONFIG_3,
        modem_config_3
    );
}

/* Configure the 16-bit LoRa preamble length. */
RA01_Status_t RA01_SetPreambleLength(uint16_t preamble_length)
{
    RA01_Status_t status;

    /* LoRa requires a preamble of at least 6 symbols. */
    if (preamble_length < 6U)
    {
        return RA01_INVALID_PARAMETER;
    }

    status = RA01_WriteRegister(
        RA01_REG_PREAMBLE_MSB,
        (uint8_t)(preamble_length >> 8U)
    );

    if (status != RA01_OK)
    {
        return status;
    }

    return RA01_WriteRegister(
        RA01_REG_PREAMBLE_LSB,
        (uint8_t)preamble_length
    );
}
/* Configure the LoRa network sync word. */
RA01_Status_t RA01_SetSyncWord(uint8_t sync_word)
{
    return RA01_WriteRegister(
        RA01_REG_SYNC_WORD,
        sync_word
    );
}

/* Configure the normal PA_BOOST output-power range. */
RA01_Status_t RA01_SetTxPower(int8_t power_dbm)
{
    uint8_t pa_config;
    uint8_t output_power;

    /* Normal PA_BOOST operation supports +2 to +17 dBm. */
    if ((power_dbm < 2) || (power_dbm > 17))
    {
        return RA01_INVALID_PARAMETER;
    }

    /* PA_BOOST relation: Pout = 2 + OutputPower. */
    output_power = (uint8_t)(power_dbm - 2);

    pa_config = RA01_PA_SELECT_BOOST |
                (output_power & RA01_OUTPUT_POWER_MASK);

    return RA01_WriteRegister(
        RA01_REG_PA_CONFIG,
        pa_config
    );
}

/* Clear selected IRQ flags by writing 1 to the corresponding bits. */
RA01_Status_t RA01_ClearIrqFlags(uint8_t flags)
{
    return RA01_WriteRegister(
        RA01_REG_IRQ_FLAGS,
        flags
    );
}

/* Configure what event appears on the DIO0 pin. */
RA01_Status_t RA01_SetDio0Mapping(RA01_Dio0Mapping_t mapping)
{
    RA01_Status_t status;
    uint8_t dio_mapping_1;

    if ((mapping != RA01_DIO0_MAP_RX_DONE) &&
        (mapping != RA01_DIO0_MAP_TX_DONE) &&
        (mapping != RA01_DIO0_MAP_CAD_DONE))
    {
        return RA01_INVALID_PARAMETER;
    }

    status = RA01_ReadRegister(
        RA01_REG_DIO_MAPPING_1,
        &dio_mapping_1
    );

    if (status != RA01_OK)
    {
        return status;
    }

    /* Clear old DIO0 mapping from bits 7:6. */
    dio_mapping_1 &= (uint8_t)(~RA01_DIO0_MAPPING_MASK);

    /* Insert the new DIO0 mapping. */
    dio_mapping_1 |= (uint8_t)mapping;

    return RA01_WriteRegister(
        RA01_REG_DIO_MAPPING_1,
        dio_mapping_1
    );
}

/* Put the radio into continuous receive mode. */
RA01_Status_t RA01_StartRxContinuous(void)
{
    RA01_Status_t status;

    status = RA01_SetMode(RA01_MODE_STANDBY);

    if (status != RA01_OK)
    {
        return status;
    }

    status = RA01_SetDio0Mapping(RA01_DIO0_MAP_RX_DONE);

    if (status != RA01_OK)
    {
        return status;
    }

    status = RA01_ClearIrqFlags(RA01_IRQ_ALL);

    if (status != RA01_OK)
    {
        return status;
    }

    status = RA01_WriteRegister(
        RA01_REG_FIFO_ADDR_PTR,
        RA01_DEFAULT_RX_FIFO_BASE
    );

    if (status != RA01_OK)
    {
        return status;
    }

    status = RA01_SetMode(RA01_MODE_RX_CONTINUOUS);

    if (status != RA01_OK)
    {
        return status;
    }

    ra01_state = RA01_STATE_RX_CONTINUOUS;

    return RA01_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
/* init function */
RA01_Status_t RA01_Init( )
{
    RA01_Status_t status;

    /*
     * Reset internal driver state.
     */
    ra01_dio0_irq_pending = 0U;
    ra01_state = RA01_STATE_STANDBY;
    ra01_rx_length = 0U;
    ra01_rx_packet_available = 0U;

    /*
     * Hardware reset for the SX1278.
     */
    RA01_Reset();

    /*
     * Check SPI connection by reading RegVersion.
     * Expected value is 0x12.
     */
    status = RA01_CheckConnection( );

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * Enable LoRa mode.
     * Important: LoRa mode must be selected while the chip is in sleep mode.
     */
    status = RA01_EnableLoRaMode();

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * Move to standby before configuring the radio.
     */
    status = RA01_SetMode(RA01_MODE_STANDBY);

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * Frequency: 433.500 MHz.
     */
    status = RA01_SetFrequency(RA01_DEFAULT_FREQUENCY_HZ);

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * FIFO memory areas.
     * RX starts from 0x00, TX starts from 0x80.
     */
    status = RA01_SetFifoBaseAddresses(
        RA01_DEFAULT_RX_FIFO_BASE,
        RA01_DEFAULT_TX_FIFO_BASE
    );

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * BW = 125 kHz
     * CR = 4/5
     * Header = explicit
     */
    status = RA01_SetModemConfig1(
        RA01_BW_125_KHZ,
        RA01_CR_4_5,
        RA01_HEADER_EXPLICIT
    );

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * SF = 7
     * CRC = enabled
     */
    status = RA01_SetModemConfig2(
        RA01_SF_7,
        RA01_CRC_ENABLED
    );

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * LDRO = disabled because SF7/BW125 has short symbol time.
     * AGC = enabled for automatic receiver gain control.
     */
    status = RA01_SetModemConfig3(
        RA01_LDRO_DISABLED,
        RA01_AGC_ENABLED
    );

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * Preamble length = 8 symbols.
     */
    status = RA01_SetPreambleLength(RA01_DEFAULT_PREAMBLE_LENGTH);

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * Sync word must match the ground station.
     */
    status = RA01_SetSyncWord(RA01_DEFAULT_SYNC_WORD);

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * TX power = 13 dBm by default.
     */
    status = RA01_SetTxPower(RA01_DEFAULT_TX_POWER_DBM);

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * Clear any old interrupt flags.
     */
    status = RA01_ClearIrqFlags(RA01_IRQ_ALL);

    if (status != RA01_OK)
    {
        return status;
    }

    /*
     * Our payload should normally listen for commands,
     * so after initialization we enter RX continuous mode.
     */
    status = RA01_StartRxContinuous();

    if (status != RA01_OK)
    {
        return status;
    }

    return RA01_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////

/* Called from HAL_GPIO_EXTI_Callback().*/
void RA01_Dio0IrqCallback(void)
{
    ra01_dio0_irq_pending = 1U;
}

/* Process pending radio events outside the interrupt. */
RA01_Status_t RA01_Process(void)
{
    RA01_Status_t status;
    uint8_t irq_flags;
    uint8_t packet_length;

    if (ra01_dio0_irq_pending == 0U)
    {
        return RA01_OK;
    }

    /*
     * Clear the software interrupt flag first.
     * If another DIO0 interrupt happens during processing,
     * the interrupt will set this flag again.
     */
    ra01_dio0_irq_pending = 0U;

    status = RA01_ReadRegister(
        RA01_REG_IRQ_FLAGS,
        &irq_flags
    );

    if (status != RA01_OK)
    {
        return status;
    }

    if ((ra01_state == RA01_STATE_RX_CONTINUOUS) &&
        ((irq_flags & RA01_IRQ_RX_DONE) != 0U))
    {
        /*
         * Read the received packet from FIFO and store it
         * inside the driver's internal RX buffer.
         */
        status = RA01_ReadReceivedPacket(
            ra01_rx_buffer,
            RA01_RX_BUFFER_SIZE,
            &packet_length
        );

        if (status != RA01_OK)
        {
            return status;
        }

        ra01_rx_length = packet_length;
        ra01_rx_packet_available = 1U;

        return RA01_OK;
    }

    if ((ra01_state == RA01_STATE_TX) &&
        ((irq_flags & RA01_IRQ_TX_DONE) != 0U))
    {
        status = RA01_ClearIrqFlags(RA01_IRQ_TX_DONE);

        if (status != RA01_OK)
        {
            return status;
        }

        /*
         * After telemetry transmission finishes,
         * immediately return to RX continuous mode.
         */
				 HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);   // Change this to your LED pin

        return RA01_StartRxContinuous();
    }

    /*
     * Unknown or unexpected interrupt.
     * Clear all flags to avoid getting stuck.
     */
    return RA01_ClearIrqFlags(RA01_IRQ_ALL);
}

/* Read multiple bytes from the SX1278 FIFO. */
static RA01_Status_t RA01_ReadFifo(uint8_t *buffer, uint8_t length)
{
    HAL_StatusTypeDef hal_status;
    uint8_t address;

    if ((buffer == NULL) || (length == 0U))
    {
        return RA01_INVALID_PARAMETER;
    }

    /* Bit 7 cleared means SPI read. */
    address = RA01_REG_FIFO & RA01_SPI_READ_MASK;

    RA01_Select();

    /* Send FIFO address first. */
    hal_status = HAL_SPI_Transmit(
        RA01_SPI_HANDLE,
        &address,
        1U,
        RA01_SPI_TIMEOUT_MS
    );

    if (hal_status == HAL_OK)
    {
        /* Read FIFO bytes while NSS remains low. */
        hal_status = HAL_SPI_Receive(
            RA01_SPI_HANDLE,
            buffer,
            length,
            RA01_SPI_TIMEOUT_MS
        );
    }

    RA01_Unselect();

    if (hal_status != HAL_OK)
    {
        return RA01_SPI_ERROR;
    }

    return RA01_OK;
}
/* Read the received LoRa packet from the FIFO. */
RA01_Status_t RA01_ReadReceivedPacket(uint8_t *buffer,uint8_t buffer_size,uint8_t *received_length)
{
    RA01_Status_t status;
    uint8_t irq_flags;
    uint8_t packet_length;
    uint8_t current_fifo_addr;

    if ((buffer == NULL) || (received_length == NULL))
    {
        return RA01_INVALID_PARAMETER;
    }

    *received_length = 0U;

    /* Check whether RxDone really happened. */
    status = RA01_ReadRegister(RA01_REG_IRQ_FLAGS, &irq_flags);

    if (status != RA01_OK)
    {
        return status;
    }

    if ((irq_flags & RA01_IRQ_RX_DONE) == 0U)
    {
        return RA01_NO_PACKET;
    }

    /* If CRC failed, discard the packet. */
    if ((irq_flags & RA01_IRQ_PAYLOAD_CRC_ERROR) != 0U)
    {
        RA01_ClearIrqFlags(
            RA01_IRQ_RX_DONE |
            RA01_IRQ_PAYLOAD_CRC_ERROR
        );

        return RA01_CRC_ERROR;
    }

    /* Get the received packet length. */
    status = RA01_ReadRegister(
        RA01_REG_RX_NB_BYTES,
        &packet_length
    );

    if (status != RA01_OK)
    {
        return status;
    }

    if (packet_length > buffer_size)
    {
        *received_length = packet_length;

        RA01_ClearIrqFlags(RA01_IRQ_RX_DONE);

        return RA01_BUFFER_TOO_SMALL;
    }

    /* Get the FIFO address where this packet starts. */
    status = RA01_ReadRegister(
        RA01_REG_FIFO_RX_CURRENT_ADDR,
        &current_fifo_addr
    );

    if (status != RA01_OK)
    {
        return status;
    }

    /* Move FIFO pointer to the start of the received packet. */
    status = RA01_WriteRegister(
        RA01_REG_FIFO_ADDR_PTR,
        current_fifo_addr
    );

    if (status != RA01_OK)
    {
        return status;
    }

    /* Read the received bytes from FIFO. */
    status = RA01_ReadFifo(buffer, packet_length);

    if (status != RA01_OK)
    {
        return status;
    }

    *received_length = packet_length;
		
		/* Return FIFO pointer to RX base for the next reception. */
		status = RA01_WriteRegister(RA01_REG_FIFO_ADDR_PTR,RA01_DEFAULT_RX_FIFO_BASE);
		
		if (status != RA01_OK)
		{
				return status;
		}


    /* Clear RxDone after the packet has been read. */
    return RA01_ClearIrqFlags(RA01_IRQ_RX_DONE);
}


/* Copy the last received packet from the driver buffer to the user buffer. */
RA01_Status_t RA01_GetReceivedPacket(uint8_t *buffer,
                                     uint8_t buffer_size,
                                     uint8_t *received_length)
{
    uint8_t i;

    if ((buffer == NULL) || (received_length == NULL))
    {
        return RA01_INVALID_PARAMETER;
    }

    *received_length = 0U;

    if (ra01_rx_packet_available == 0U)
    {
        return RA01_NO_PACKET;
    }

    if (ra01_rx_length > buffer_size)
    {
        *received_length = ra01_rx_length;
        return RA01_BUFFER_TOO_SMALL;
    }

    for (i = 0U; i < ra01_rx_length; i++)
    {
        buffer[i] = ra01_rx_buffer[i];
    }

    *received_length = ra01_rx_length;

    /*
     * Packet has been taken by the application.
     * Clear the available flag.
     */
    ra01_rx_packet_available = 0U;

    return RA01_OK;
}

/* Write multiple bytes into the SX1278 FIFO. */
static RA01_Status_t RA01_WriteFifo(const uint8_t *data, uint8_t length)
{
    HAL_StatusTypeDef hal_status;
    uint8_t address;

    if ((data == NULL) || (length == 0U))
    {
        return RA01_INVALID_PARAMETER;
    }

    /* Bit 7 set means SPI write. */
    address = RA01_REG_FIFO | RA01_SPI_WRITE_BIT;

    RA01_Select();

    /* Send FIFO register address first. */
    hal_status = HAL_SPI_Transmit(
        RA01_SPI_HANDLE,
        &address,
        1U,
        RA01_SPI_TIMEOUT_MS
    );

    if (hal_status == HAL_OK)
    {
        /* Write the packet bytes while NSS remains low. */
        hal_status = HAL_SPI_Transmit(
            RA01_SPI_HANDLE,
            (uint8_t *)data,
            length,
            RA01_SPI_TIMEOUT_MS
        );
    }

    RA01_Unselect();

    if (hal_status != HAL_OK)
    {
        return RA01_SPI_ERROR;
    }

    return RA01_OK;
}

/* Start non-blocking LoRa transmission. */
RA01_Status_t RA01_TransmitAsync(const uint8_t *data, uint8_t length)
{
    RA01_Status_t status;

    if ((data == NULL) || (length == 0U))
    {
        return RA01_INVALID_PARAMETER;
    }

    if (ra01_state == RA01_STATE_TX)
    {
        return RA01_BUSY;
    }

    /* Standby is used before loading the TX FIFO. */
    status = RA01_SetMode(RA01_MODE_STANDBY);

    if (status != RA01_OK)
    {
        return status;
    }

    /* During TX, DIO0 should rise when transmission is done. */
    status = RA01_SetDio0Mapping(RA01_DIO0_MAP_TX_DONE);

    if (status != RA01_OK)
    {
        return status;
    }

    /* Clear old IRQ flags before starting a new transmission. */
    status = RA01_ClearIrqFlags(RA01_IRQ_ALL);

    if (status != RA01_OK)
    {
        return status;
    }

    /* Start writing TX packet at the TX FIFO base address. */
    status = RA01_WriteRegister(
        RA01_REG_FIFO_ADDR_PTR,
        RA01_DEFAULT_TX_FIFO_BASE
    );

    if (status != RA01_OK)
    {
        return status;
    }

    /* Copy packet bytes into the SX1278 FIFO. */
    status = RA01_WriteFifo(data, length);

    if (status != RA01_OK)
    {
        return status;
    }

    /* Tell the SX1278 how many bytes must be transmitted. */
    status = RA01_WriteRegister(
        RA01_REG_PAYLOAD_LENGTH,
        length
    );

    if (status != RA01_OK)
    {
        return status;
    }

    /* Enter TX mode. The function returns immediately. */
    status = RA01_SetMode(RA01_MODE_TX);

    if (status != RA01_OK)
    {
        return status;
    }

    ra01_state = RA01_STATE_TX;

    return RA01_OK;
}