/*
This software is subject to the license described in the license.txt file
included with this software distribution. You may not use this file except in compliance
with this license.

Copyright (c) Dynastream Innovations Inc. 2014
All rights reserved.
*/

#include <stdint.h>
#include <string.h>
#include "asc_coordinator.h"
#include "asc_events.h"
#include "asc_parameters.h"
#include "asc_master_to_master.h"
#include "asc_master.h"
#include "ant_phone_connection.h"
#include "ant_interface.h"
#if 1 // @todo: remove me
#include "nordic_common.h" //Included here because softdevice_handler.h requires it but does not include it, itself
#include "softdevice_handler.h"
#endif // 0
#include "ant_parameters.h"
#include "app_error.h"
#include "leds.h"
#include "boards.h"
#include "ant_stack_config.h"

// Global channel parameters
#define SERIAL_NUMBER_ADDRESS           ((uint32_t) 0x10000060)     /**< FICR + 60 */
#define ANT_CHANNEL_DEFAULT_NETWORK     0x00                        /**< ANT Channel Network. */

// Channel configuration for mobile phone interface.
#define ANT_MOBILE_CHANNEL              ((uint8_t) 0)               /**< Mobile phone interface channel - ANT Channel 0. */
#define ANT_MOBILE_CHANNEL_TYPE         CHANNEL_TYPE_MASTER         /**< Mobile phone channel type = master. */
#define ANT_MOBILE_CHANNEL_PERIOD       ((uint16_t) 8192)           /**< Mobile phone interface channel period 4 Hz. */
#define ANT_MOBILE_FREQUENCY            ((uint8_t) 77)              /**< Mobile phone frequency 2477MHz. */

#define ANT_MOBILE_DEVICE_NUMBER        ((uint16_t) (*(uint32_t *)SERIAL_NUMBER_ADDRESS) & 0xFFFF)    /**< Mobile phone channel id device number = serial number of the device. */
#define ANT_MOBILE_DEVICE_TYPE          ((uint8_t) 2)               /**< Mobile phone channel id device type */
#define ANT_MOBILE_TRANSMISSION_TYPE    ((uint8_t) 5)               /**< Mobile phone channel id transmission type. */

#define ANT_MOBILE_MAIN_PAGE            ((uint8_t) 1)               /**< Main status page for mobile interface channel. */
#define ANT_MOBILE_COMMAND_PAGE         ((uint8_t) 2)               /**< Command page for mobile interface (from mobile to device). */


// Channel configuration for device to device master and slave relay channel interface.
#define ANT_RELAY_MASTER_CHANNEL       ((uint8_t) 1)                  /**< Device to device master channel - ANT Channel 1. */
#define ANT_RELAY_SLAVE_CHANNEL        ((uint8_t) 2)                  /**< Device to device slave channel - ANT Channel 2. */
#define ANT_RELAY_MASTER_CHANNEL_TYPE  CHANNEL_TYPE_MASTER            /**< Device to device master channel type = master. */
#define ANT_RELAY_SLAVE_CHANNEL_TYPE   CHANNEL_TYPE_SLAVE             /**< Device to device slave channel type = slave. */
#define ANT_RELAY_CHANNEL_PERIOD       ((uint16_t) 8192)              /**< Device to device channel period 4 Hz. */
#define ANT_RELAY_FREQUENCY            ((uint8_t) 72)                 /**< Device to device frequency 2472MHz. */

#define ANT_RELAY_MASTER_DEVICE_NUMBER ANT_MOBILE_DEVICE_NUMBER       /**< Device to device master channel id device number = serial number of the device. */
#define ANT_RELAY_SLAVE_DEVICE_NUMBER  ((uint16_t) 0)                 /**< Device to device channel id device number = wildcard. */
#define ANT_RELAY_DEVICE_TYPE          ((uint8_t) 1)                  /**< Device to device channel id device type */
#define ANT_RELAY_TRANSMISSION_TYPE    ((uint8_t) 5)                  /**< Device to device channel id transmission type. */
#define ANT_PROXIMITY_BIN              ((uint8_t) 1)                  /**< Proximity bin for slave side of relay channel. */

#define ANT_RELAY_MAIN_PAGE             ((uint8_t) 1)                 /**< Main status page for relay interface channel. */


#define APP_TIMER_PRESCALER           0                                         /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE       2u                                        /**< Size of timer operation queues. */

#define BUTTON_DETECTION_DELAY        APP_TIMER_TICKS(50u, APP_TIMER_PRESCALER) /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

typedef enum
{
    ANT_COMMAND_PAIRING = 1,
    ANT_COMMAND_ON = 2,
    ANT_COMMAND_OFF = 3
} ant_state_command_t;

// Static variables and buffers.
static uint8_t pm_ant_public_key[8] = {0xE8, 0xE4, 0x21, 0x3B, 0x55, 0x7A, 0x67, 0xC1}; // Public Network Key
static uint32_t m_led_change_counter = 0;
static uint8_t m_broadcast_data[ANT_STANDARD_DATA_PAYLOAD_SIZE]; /**< Primary data transmit buffer. */

/**@brief Function for setting up the ANT channels
 *ANT初始化
 */
static void ant_channel_setup(void)
{
    uint32_t err_code;

    // !! GLOBAL ANT CONFIGURATION !! //
    err_code = sd_ant_network_address_set(ANT_CHANNEL_DEFAULT_NETWORK, pm_ant_public_key);
    APP_ERROR_CHECK(err_code);
    // !! CONFIGURE RELAY MASTER CHANNEL !! //
    
    /* The relay master channel is always on and transmits the 
       status of the LED and the status of the slave relay channel
       (open/closed/searching). It is 100% bi-directional once
       a slave connects to it (status updates from the slave are 
       sent on every message period)
    */

    //Assign the relay master channel type
    err_code = sd_ant_channel_assign(   ANT_RELAY_MASTER_CHANNEL,
                                        ANT_RELAY_MASTER_CHANNEL_TYPE,
                                        ANT_CHANNEL_DEFAULT_NETWORK,
                                        0);
    APP_ERROR_CHECK(err_code);

     //Assign channel id
    err_code = sd_ant_channel_id_set(   ANT_RELAY_MASTER_CHANNEL,
                                        ANT_RELAY_MASTER_DEVICE_NUMBER,
                                        ANT_RELAY_DEVICE_TYPE,
                                        ANT_RELAY_TRANSMISSION_TYPE);
    APP_ERROR_CHECK(err_code);

    //Assign channel frequency
    err_code = sd_ant_channel_radio_freq_set(   ANT_RELAY_MASTER_CHANNEL,
                                                ANT_RELAY_FREQUENCY);
    APP_ERROR_CHECK(err_code);

   //Assign channel message period
    err_code = sd_ant_channel_period_set (  ANT_RELAY_MASTER_CHANNEL,
                                            ANT_RELAY_CHANNEL_PERIOD);
    APP_ERROR_CHECK(err_code);

    // Open channel right away.
    err_code = sd_ant_channel_open(ANT_RELAY_MASTER_CHANNEL);
    APP_ERROR_CHECK(err_code);

    // !! CONFIGURE RELAY SLAVE CHANNEL !! //
    
    /* The purpose of the relay slave channel is to find and synchronize 
       to another devices master really channel. The slave channel is only
       opened on a button press and uses proximity pairing to connect to a 
       master channel. Once tracking a master the slave channel will send status
       message back to the master 100% of the time. 
    */

     //Assign the relay slave channel type
    err_code = sd_ant_channel_assign(   ANT_RELAY_SLAVE_CHANNEL,
                                        ANT_RELAY_SLAVE_CHANNEL_TYPE,
                                        ANT_CHANNEL_DEFAULT_NETWORK,
                                        0);
    APP_ERROR_CHECK(err_code);

     //Assign channel id
    err_code = sd_ant_channel_id_set(   ANT_RELAY_SLAVE_CHANNEL,
                                        ANT_RELAY_SLAVE_DEVICE_NUMBER,
                                        ANT_RELAY_DEVICE_TYPE,
                                        ANT_RELAY_TRANSMISSION_TYPE);
    APP_ERROR_CHECK(err_code);

    //Assign channel frequency
    err_code = sd_ant_channel_radio_freq_set(   ANT_RELAY_SLAVE_CHANNEL,
                                                ANT_RELAY_FREQUENCY);
    APP_ERROR_CHECK(err_code);

   //Assign channel message period
    err_code = sd_ant_channel_period_set (  ANT_RELAY_SLAVE_CHANNEL,
                                            ANT_RELAY_CHANNEL_PERIOD);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ant_prox_search_set(ANT_RELAY_SLAVE_CHANNEL, ANT_PROXIMITY_BIN, 0);
    APP_ERROR_CHECK(err_code);
	
	err_code = sd_ant_channel_open(ANT_RELAY_SLAVE_CHANNEL);
	APP_ERROR_CHECK(err_code);
    // DO NOT OPEN THE SLAVE RIGHT AWAY - IT OPENS ON BUTTON PRESS
    // OR MESSAGE FROM MOBILE PHONE
}
// Public Functions

#include "ant_scalable_rx.h"
#include "ant_scalable_tx.h"
void ascc_init(void)
{
	uint32_t err_code;
	
	
	// Setup SoftDevice and events handler
    err_code = softdevice_ant_evt_handler_set(ant_scaleable_event_handler);
    APP_ERROR_CHECK(err_code);
	
	err_code = ant_stack_static_config();
	APP_ERROR_CHECK(err_code);
	
	ant_scaleable_channel_rx_broadcast_setup();
	ant_scaleable_channel_tx_broadcast_setup();
	//ant_channel_setup();
}



/**@brief Decode and handle the main relay page.
 *  set the LED if required.         
 *接收到数据回调
 * @param[in] p_payload ANT message 8-byte payload.
 */
void ant_handle_main_page2(uint8_t* p_payload)
{               
    uint32_t counter =  (uint32_t)p_payload[2] |
                        (uint32_t)(p_payload[3] << 8) |
                        (uint32_t)(p_payload[3] << 16) |
                        (uint32_t)(p_payload[3] << 24);

    // If counter changed, set the led to what 
    // we received in the message. 
    if(counter > m_led_change_counter)
    {
		uint8_t led_state = p_payload[1];
		LEDS_INVERT(BSP_LED_5_MASK);
		//        if(led_state == 1)
		//            LEDS_ON(BSP_LED_5_MASK);
		//        else
		//            LEDS_OFF(BSP_LED_5_MASK);
		m_led_change_counter = counter;
    }
}

/**@brief Function for creating and sending main
 * relay data page.
 *发送数据
 * @param[in] channel ANT channel on which to send this message.
 */
void ant_relay_main_message2(uint8_t channel)
{
	uint8_t status;
	uint32_t err_code = sd_ant_channel_status_get(ANT_RELAY_SLAVE_CHANNEL, &status);
	APP_ERROR_CHECK(err_code);

	m_broadcast_data[0] = ANT_RELAY_MAIN_PAGE;
	m_broadcast_data[1] = ( LED_IS_ON(BSP_LED_0_MASK) )? 1 : 0;
	m_broadcast_data[2] = (uint8_t)(m_led_change_counter >> 0);
	m_broadcast_data[3] = (uint8_t)(m_led_change_counter >> 8);
	m_broadcast_data[4] = (uint8_t)(m_led_change_counter >> 16);
	m_broadcast_data[5] = (uint8_t)(m_led_change_counter >> 24);
	m_broadcast_data[6] = 0xFF;
	m_broadcast_data[7] = status & STATUS_CHANNEL_STATE_MASK;

	err_code = sd_ant_broadcast_message_tx(channel, ANT_STANDARD_DATA_PAYLOAD_SIZE, m_broadcast_data);
	APP_ERROR_CHECK(err_code);
}

/**@brief Process ANT message on ANT slave relay channel
 *
 * @param[in] p_ant_event ANT message content.
 */
void ant_process_relay_slave(ant_evt_t* p_ant_event)
{
    static bool first_recieved = false;
    ANT_MESSAGE* p_ant_message = (ANT_MESSAGE*)p_ant_event->evt_buffer;

    switch(p_ant_event->event)
    {
        case EVENT_RX:
        {
            switch(p_ant_message->ANT_MESSAGE_aucPayload[0])
            {
                case ANT_RELAY_MAIN_PAGE:
                {
                    ant_handle_main_page2(p_ant_message->ANT_MESSAGE_aucPayload);                    
                    break;
                }
            }

            LEDS_ON(BSP_LED_1_MASK);

            if(first_recieved)
                break;
            else
                first_recieved = true;

            //!!INTENTIONAL FALL THROUGH !!
        }
        // fall-through
        case EVENT_TX:
        {
            ant_relay_main_message2(ANT_RELAY_SLAVE_CHANNEL);
            break;
        }
        case EVENT_RX_SEARCH_TIMEOUT:
        {
            // Channel has closed.
            // Re-initialize proximity search settings. 
            uint32_t err_code = sd_ant_prox_search_set(ANT_RELAY_SLAVE_CHANNEL, ANT_PROXIMITY_BIN, 0);
            APP_ERROR_CHECK(err_code);
            LEDS_OFF(BSP_LED_5_MASK);
            break;
        }
        default:
        {
            break;
        }

    }
}

void ascmm_send(uint8_t cmd) {
	//m_tx_buffer[3] = 3;
	//ascmm_relay_message(m_tx_buffer, ASCMM_RETRIES);
	// Open slave channel to 
	if(cmd == 0) {
		uint8_t channel_status;
		uint32_t err_code = sd_ant_channel_status_get (ANT_RELAY_SLAVE_CHANNEL, &channel_status);
		APP_ERROR_CHECK(err_code);

		if((channel_status & STATUS_CHANNEL_STATE_MASK) == STATUS_ASSIGNED_CHANNEL)
		{
				err_code = sd_ant_channel_open(ANT_RELAY_SLAVE_CHANNEL);
				APP_ERROR_CHECK(err_code);
		}
	} else {
		// Toggle the ste of the LED
		m_led_change_counter++;
		LEDS_INVERT(BSP_LED_5_MASK);
		ant_relay_main_message2(ANT_RELAY_MASTER_CHANNEL);
	}
}

/**@brief Process ANT message on ANT relay master channel
 *
 * @param[in] p_ant_event ANT message content.
 */
void ant_process_relay_master(ant_evt_t* p_ant_event)
{
    ANT_MESSAGE* p_ant_message = (ANT_MESSAGE*)p_ant_event->evt_buffer;
    switch(p_ant_event->event)
    {
        case EVENT_RX:
        {
            switch(p_ant_message->ANT_MESSAGE_aucPayload[0])
            {
                case ANT_RELAY_MAIN_PAGE:
                {
                    ant_handle_main_page2(p_ant_message->ANT_MESSAGE_aucPayload);                   
                    break;
                }
            }
            break;
        }
        case EVENT_TX:
        {
            ant_relay_main_message2(ANT_RELAY_MASTER_CHANNEL);
            break;   
        }
        default:
        {
            break;
        }
    }
}

// ANT event message buffer.
static ant_evt_t ant_event;

void ascc_poll_for_ant_evets(void) {	
	uint32_t err_code;
	// Extract and process all pending ANT events as long as there are any left.
	do
	{
		// Fetch the event.
		err_code = sd_ant_event_get(&ant_event.channel, &ant_event.event, ant_event.evt_buffer);
		if (err_code == NRF_SUCCESS)
		{
			switch(ant_event.channel)
			{
				case ANT_RELAY_MASTER_CHANNEL:
				{
					ant_process_relay_master(&ant_event);
					break;
				}
				case ANT_RELAY_SLAVE_CHANNEL:
				{
					ant_process_relay_slave(&ant_event);
					break;
				}
				case ANT_MOBILE_CHANNEL:
				{
					//  ant_process_mobile(&ant_event);
					break;
				}
				default:
				{
					break;
				}
			}
		}
	}
	while (err_code == NRF_SUCCESS);
}
