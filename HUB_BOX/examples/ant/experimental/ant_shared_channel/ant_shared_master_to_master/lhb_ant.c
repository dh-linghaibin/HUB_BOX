

#include <stdint.h>
#include <string.h>
#include "softdevice_handler.h"
#include "ant_stack_config.h"
#include "ant_scalable_rx.h"
#include "ant_scalable_tx.h"
#include "lhb_ant.h"

#include "ant_interface.h"
#include "ant_parameters.h"
#include "pstorage.h"

static uint8_t m_broadcast_data[ANT_STANDARD_DATA_PAYLOAD_SIZE] = { 0 };    /**< Primary data transmit buffers. */
extern uint8_t  p_data4[50];
extern uint8_t  ble_rx_ok;

void AntInit(void)
{
	uint32_t err_code;
	
	
	// Setup SoftDevice and events handler
    err_code = softdevice_ant_evt_handler_set(ant_scaleable_event_handler);
    APP_ERROR_CHECK(err_code);
	
	err_code = ant_stack_static_config();
	APP_ERROR_CHECK(err_code);
	
	ant_scaleable_channel_rx_broadcast_setup();
	ant_scaleable_channel_tx_broadcast_setup();
}


static uint8_t tt = 0;

void AntSend(uint8_t cmd,uint8_t data1,uint8_t data2,uint8_t data3,uint8_t data4,uint8_t data5,uint8_t data6) {
	uint32_t err_code;
	for(int i = 0; i < ANT_STANDARD_DATA_PAYLOAD_SIZE;i++) {
		m_broadcast_data[i]  = 0x00;
	}
	m_broadcast_data[0] = cmd;
	m_broadcast_data[1] = data1;
	m_broadcast_data[2] = data2;
	m_broadcast_data[3] = data3;
	m_broadcast_data[4] = data4;
	m_broadcast_data[5] = data5;
	m_broadcast_data[6] = data6;
	if(tt == 0) {
		tt = 1;
	} else {
		tt = 0;
	}
	m_broadcast_data[7] = tt;
	err_code = sd_ant_broadcast_message_tx(1,
							   ANT_STANDARD_DATA_PAYLOAD_SIZE,
							   m_broadcast_data);
	APP_ERROR_CHECK(err_code);
}

uint8_t but_led[4] = {1,2,3,4};
uint8_t but_ble[4] = {0,4,5,7};
uint8_t but_por[4] = {1,2,3,4};

#include "app_uart.h"
#include "app_util_platform.h"

uint8_t  p_data[50];
extern uint8_t len;
uint8_t  ant_data[10];

void AntTask(void) {
	if(AntRxGetFlag() == 1) {
		AntRxSetFlag();
		AntPush();
		switch(AntRxGetData(0)){
			case 0x01:
				ant_data[0] = 0x55;
				ant_data[1] = 0x01;
				ant_data[2] = AntRxGetData(1);
				ant_data[3] = AntRxGetData(2);
				for (uint32_t i = 0; i < 4; i++)
				{
					while(app_uart_put(ant_data[i]) != NRF_SUCCESS);
	
				}
			break;
			case 0x02:
				ant_data[0] = 0x55;
				ant_data[1] = 0x02;
				ant_data[2] = AntRxGetData(1);
				ant_data[3] = AntRxGetData(2);
				for (uint32_t i = 0; i < 4; i++)
				{
					while(app_uart_put(ant_data[i]) != NRF_SUCCESS);
	
				}
			break;
			case 0x03:
				ant_data[0] = 0x55;
				ant_data[1] = 0x03;
				ant_data[2] = AntRxGetData(1);
				ant_data[3] = AntRxGetData(2);
				for (uint32_t i = 0; i < 4; i++)
				{
					while(app_uart_put(ant_data[i]) != NRF_SUCCESS);
	
				}
			break;
		}
	}
}

void BleTask(void) {
	if(ble_rx_ok == 1) {
		ble_rx_ok = 0;
		if(p_data4[0] == 0x3a) {
			switch(p_data4[1]) {
				case 0x05://设置功能
					AntSend(0x01,p_data4[2],p_data4[3],p_data4[4],p_data4[5],0x00,0x00);
					break;
				case 0x07://拍照设置
					AntSend(0x02,p_data4[2],p_data4[3],p_data4[4],p_data4[5],0x00,0x00);
					break;
				case 0x08:
					AntSend(0x03,p_data4[2],p_data4[3],p_data4[4],p_data4[5],0x00,0x00);
					break;
				case 0x09:
					AntSend(0x04,p_data4[2],p_data4[3],p_data4[4],p_data4[5],0x00,0x00);
					break;
				default:
					for (uint32_t i = 0; i < len; i++)
					{
						while(app_uart_put(p_data4[i]) != NRF_SUCCESS);
		
					}
					break;
			}
		}
	}
}


