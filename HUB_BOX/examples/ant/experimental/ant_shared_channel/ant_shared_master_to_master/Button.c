
#include "nrf_gpio.h"
#include "Button.h"

void ButtonInit(void) {
	nrf_gpio_cfg_input(0,NRF_GPIO_PIN_PULLUP);
}


uint8_t ButtonRead(void) {
	static uint16_t but = 0;
	if(nrf_gpio_pin_read(0) == 0) {
		if(but < 50000) {
			but++;
			if(but == 49999) {
				return 0x02;
			}
		}
	} else {
		if(but > 49999) {
			but = 0;
			return 0x01;
		}
		but = 0;
	}
	return 0x00;
}

