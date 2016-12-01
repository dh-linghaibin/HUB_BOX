/*
This software is subject to the license described in the license.txt file included with this software distribution.
You may not use this file except in compliance with this license.

Copyright © Dynastream Innovations Inc. 2015
All rights reserved.
*/

#include <stdint.h>
#include "ant_interface.h"
#include "ant_parameters.h"
#include "app_error.h"
#include "boards.h"
#include "ant_stack_config_defs.h"
#include "ant_channel_config.h"
#include "ant_scalable_tx.h"

// Channel configuration.
#define ANT_EXT_ASSIGN                  0x00                            /**< ANT Ext Assign. */

// Channel ID configuration.
#define CHAN_ID_DEV_TYPE                0x02u                           /**< Device type. */
#define CHAN_ID_TRANS_TYPE              0x01u                           /**< Transmission type. */

// Channel Period configuration
#define CHAN_PERIOD                     8192                            /**< Channel Period. */

// RF Frequency configuration
#define RF_FREQ                         66                              /**< RF Freq. */

// Miscellaneous defines.
#define ANT_CHANNEL_DEFAULT_NETWORK     0x00                            /**< ANT Channel Network. */

// Private variables
static uint8_t m_counter[ANT_CONFIG_TOTAL_CHANNELS_ALLOCATED]   = { 0u };   /**< Counters to increment the ANT broadcast data payload. */
static uint8_t m_broadcast_data[ANT_STANDARD_DATA_PAYLOAD_SIZE] = { 0 };    /**< Primary data transmit buffers. */
static uint8_t m_num_open_channels                              = 0;        /**< Number of channels open */

/**@brief Function to display the bottom nibble of the input byte
 */
static void ant_scaleable_display_byte_on_leds(uint8_t byte_to_display)
{
    LEDS_OFF(LEDS_MASK);

    uint32_t mask = 0;
    if ((byte_to_display & 0x01) == 1)
    {
        mask = mask | BSP_LED_3_MASK;
    }
    if (((byte_to_display >> 1) & 0x01) == 1)
    {
        mask = mask | BSP_LED_2_MASK;
    }
    if (((byte_to_display >> 2) & 0x01) == 1)
    {
        mask = mask | BSP_LED_1_MASK;
    }
    if (((byte_to_display >> 3) & 0x01) == 1)
    {
        mask = mask | BSP_LED_0_MASK;
    }

    LEDS_ON(mask);
}


void ant_scaleable_channel_tx_broadcast_setup(void)
{
    uint32_t err_code;
    uint32_t i = 2;

    ant_channel_config_t channel_config2 =
    {
        .channel_number     = i,
        .channel_type       = CHANNEL_TYPE_MASTER_TX_ONLY,
        .ext_assign         = ANT_EXT_ASSIGN,
        .rf_freq            = RF_FREQ,
        .transmission_type  = CHAN_ID_TRANS_TYPE,
        .device_type        = CHAN_ID_DEV_TYPE,
        .device_number      = i+1,
        .channel_period     = CHAN_PERIOD,
        .network_number     = ANT_CHANNEL_DEFAULT_NETWORK,
    };

  //  for (i = 0; i < ANT_CONFIG_TOTAL_CHANNELS_ALLOCATED; i++)
   // {
        // Configure channel
        channel_config2.channel_number = 2;
        channel_config2.device_number  = 3;

        err_code = ant_channel_init(&channel_config2);
        APP_ERROR_CHECK(err_code);
		
		// Channel period = 1000/32768s, but do we need to set this for async Tx???
		err_code = sd_ant_channel_period_set(i, 8192 );
		APP_ERROR_CHECK(err_code);

		// Set Tx power = 0dBm
		err_code = sd_ant_channel_radio_tx_power_set(i, RADIO_TX_POWER_LVL_4, 0 );
		APP_ERROR_CHECK(err_code);
		
        // Open channel
        err_code = sd_ant_channel_open(i);
        APP_ERROR_CHECK(err_code);

      //  m_num_open_channels++;
        //ant_scaleable_display_byte_on_leds(m_num_open_channels);

		//-------------------------------------------------------------
		i = 3;
		channel_config2.channel_number = 3;
        channel_config2.device_number  = 4;

        err_code = ant_channel_init(&channel_config2);
        APP_ERROR_CHECK(err_code);
		
//		// Channel period = 1000/32768s, but do we need to set this for async Tx???
		err_code = sd_ant_channel_period_set(i, 8192 );
		APP_ERROR_CHECK(err_code);

//		// Set Tx power = 0dBm
		err_code = sd_ant_channel_radio_tx_power_set(i, RADIO_TX_POWER_LVL_4, 0 );
		APP_ERROR_CHECK(err_code);
        // Open channel
        err_code = sd_ant_channel_open(i);
        APP_ERROR_CHECK(err_code);
   // }
}


void ant_scaleable_event_handlertx(ant_evt_t * p_ant_evt)
{
    uint32_t err_code;

    switch (p_ant_evt->event)
    {
        // ANT broadcast success.
        // Increment the counter and send a new broadcast.
        case EVENT_TX:
            // Increment the counter.
            m_counter[p_ant_evt->channel]++;

            // Assign a new value to the broadcast data.
            m_broadcast_data[ANT_STANDARD_DATA_PAYLOAD_SIZE - 1] = m_counter[p_ant_evt->channel];

            // Broadcast the data.
            err_code = sd_ant_broadcast_message_tx(p_ant_evt->channel,
                                                   ANT_STANDARD_DATA_PAYLOAD_SIZE,
                                                   m_broadcast_data);
            APP_ERROR_CHECK(err_code);

            break;

        default:
            break;
    }
}

  
void tttx(void) {
	uint32_t err_code;
	for(int i = 0; i < ANT_STANDARD_DATA_PAYLOAD_SIZE;i++) {
		m_broadcast_data[i]  = 0x44;
	}
	err_code = sd_ant_broadcast_message_tx(2,
							   ANT_STANDARD_DATA_PAYLOAD_SIZE,
							   m_broadcast_data);
	APP_ERROR_CHECK(err_code);
}
