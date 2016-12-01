
#include "lhb_ant.h"
#include <stdint.h>
#include <string.h>
#include "asc_coordinator.h"
#include "asc_events.h"
#include "asc_parameters.h"
#include "asc_master.h"
#include "ant_interface.h"
#if 1 // @todo: remove me
#include "nordic_common.h" //Included here because softdevice_handler.h requires it but does not include it, itself
#include "softdevice_handler.h"
#endif // 0
#include "ant_parameters.h"
#include "app_error.h"
#include "boards.h"
#include "ant_stack_config.h"
#include "asc_master_to_master.h"

//Generic Channel properties
#define ANT_PUBLIC_NETWORK_NUMBER       0x00u                       /**< ANT Public/Default Network Key. */
#define ANT_CUSTOM_TRANSMIT_POWER       0u                          /**< ANT Custom Transmit Power (Invalid/Not Used). */
#define SERIAL_NUMBER_ADDRESS           ((uint32_t) 0x10000060)     // FICR + 60
#define DEVICE_NUMBER                   (*(uint16_t*) SERIAL_NUMBER_ADDRESS)

// ASCM Channel configuration.
#define ASCM_CHANNEL                    0x00u                       /**< ASC Master channel. */
#define ASCM_RF_FREQUENCY               0x42u                       /**< 2400 + 66Mhz. */
#define ASCM_CHANNEL_PERIOD             MSG_PERIOD_4HZ              /**< ASC Master channel period. */
#define ASCM_CHANNEL_TYPE               CHANNEL_TYPE_MASTER         /**< ASC Master channel type. */
#ifdef TWO_BYTE_SHARED_ADDRESS
    #define ASCM_TRANS_TYPE             ANT_TRANS_TYPE_2_BYTE_SHARED_ADDRESS    /**< Two byte shared address transmission type. */
#else
    #define ASCM_TRANS_TYPE             ANT_TRANS_TYPE_1_BYTE_SHARED_ADDRESS    /**< One byte shared address transmission type. */
#endif
#define ASCM_DEVICE_TYPE                0x02u                       /**< Device type. */
#define DEFAULT_RETRIES                 2u                          /**< The default number of retries that this demo will tell an ASC Master to attempt when sending a command to all slaves. */



#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                            /**< Whether or not to include the service_changed characteristic. If not enabled, the server's database cannot be changed for the lifetime of the device */



// Master to Master Channel configuration.
#define ASCMM_DISCOVERY_CHANNEL         0x02u                       /**< Master to Master device discovery channel. */
#define ASCMM_CHANNEL                   0x03u                       /**< Master to Master channel. */
#define ASCMM_RF_FREQUENCY              0x42u                       /**< Master to Master rf frequency offset (same as PHONE_RF_FREQUENCY since the phone connection channel is used for discovery). */
#define ASCMM_CHANNEL_PERIOD            MSG_PERIOD_4HZ              /**< Master to Master channel period (same as PHONE_CHANNEL_PERIDO since the phone connection channel is used for discovery). */
#define ASCMM_CHANNEL_TYPE              CHANNEL_TYPE_SLAVE          /**< Master to Master channel type. */
#define ASCMM_TRANS_TYPE                0x05u                       /**< Master to Master  transmission type (same as PHONE_TRANS_TYPE since the phone connection channel is used for discovery). */
#define ASCMM_DEVICE_TYPE               0x04u                       /**< Master to Master device type (same as PHONE_DEVICE_TYPE since the phone connection channel is used for discovery). */
#define ASCMM_RETRIES                   0x02                        /**< Number of messages retries that will be sent when relaying messages. */

// Phone Channel configuration.
#define PHONE_CHANNEL                   0x01u                       /**< Phone connection channel. */
#define PHONE_RF_FREQUENCY              0x42u                       /**< Phone connection rf frequency offset. */
#define PHONE_CHANNEL_PERIOD            MSG_PERIOD_4HZ              /**< Phone connection channel period. */
#define PHONE_CHANNEL_TYPE              CHANNEL_TYPE_MASTER         /**< Phone connection channel type. */
#define PHONE_TRANS_TYPE                0x05u                       /**< Phone connection transmission type. */
#define PHONE_DEVICE_TYPE               0x03u                       /**< Phone connection device type. */

// Static variables and buffers.
static uint16_t         m_neighbor_id = INVALID_NEIGHBOUR_ID;
static bool             m_is_reporting_mode_on = true;
static uint8_t          m_ant_public_network_key[] = {0xE8, 0xE4, 0x21, 0x3B, 0x55, 0x7A, 0x67, 0xC1}; /**< ANT Public/Default Network Key. */
static uint8_t          m_tx_buffer[ANT_STANDARD_DATA_PAYLOAD_SIZE] = {0};

const asc_ant_params_t  m_asc_parameters = {
    ASCM_CHANNEL,
    ANT_PUBLIC_NETWORK_NUMBER,
    (uint16_t*) SERIAL_NUMBER_ADDRESS,
    ASCM_DEVICE_TYPE,
    ASCM_TRANS_TYPE,
    ASCM_CHANNEL_PERIOD,
    ASCM_CHANNEL_TYPE,
    ASCM_RF_FREQUENCY,
    RADIO_TX_POWER_LVL_3
}; /**< Structure containing setup parameters for the auto shared master. */
const asc_ant_params_t  m_ascmm_discovery_parameters = {
    ASCMM_DISCOVERY_CHANNEL,
    ANT_PUBLIC_NETWORK_NUMBER,
    (uint16_t*) SERIAL_NUMBER_ADDRESS,
    PHONE_DEVICE_TYPE,
    PHONE_TRANS_TYPE,
    PHONE_CHANNEL_PERIOD,
    CHANNEL_TYPE_SLAVE,
    PHONE_RF_FREQUENCY,
    RADIO_TX_POWER_LVL_3
}; /**< Structure containing setup parameters for the discovery portion of the master-to-master connection.
        In this case, it searches for the phone connection channel. */

const asc_ant_params_t  m_ascmm_connection_parameters = {
    ASCMM_CHANNEL,
    ANT_PUBLIC_NETWORK_NUMBER,
    (uint16_t*) SERIAL_NUMBER_ADDRESS,
    ASCMM_DEVICE_TYPE,
    ASCMM_TRANS_TYPE,
    ASCMM_CHANNEL_PERIOD,
    ASCMM_CHANNEL_TYPE,
    ASCMM_RF_FREQUENCY,
    RADIO_TX_POWER_LVL_3
}; /**< Structure containing setup parameters for the final master-to-master connection. */


void ant_int(void) {
		uint32_t err_code;
	
		SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC, false);
	
    err_code = ant_stack_static_config();
    APP_ERROR_CHECK(err_code);

    // Configure the network key for our chosen netowrk number
    err_code = sd_ant_network_address_set(ANT_PUBLIC_NETWORK_NUMBER, m_ant_public_network_key);
    APP_ERROR_CHECK(err_code);
    
    // Initialise and start the asc, ascmm, and phone modules
    ascm_init(&m_asc_parameters);
    ascmm_init(&m_ascmm_discovery_parameters, &m_ascmm_connection_parameters, DEVICE_NUMBER);
    //phc_init(&m_phone_parameters);

    ascm_turn_on();
    ascmm_turn_on();
}


