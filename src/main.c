
#include <delay.h>
#include <LedConfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x.h>

#include "main.h"
#include "station_config.h"

#include "diag/Trace.h"
#include "antilib_adc.h"
#include "afsk_pr.h"
#include "drivers/serial.h"
#include "TimerConfig.h"
#include "PathConfig.h"

#include "aprs/digi.h"
#include "aprs/telemetry.h"
#include "aprs/dac.h"
#include "aprs/beacon.h"

#include "rte_wx.h"

//#include "Timer.h"
//#include "BlinkLed.h"

#ifdef _METEO
#include <wx_handler.h>
#include "drivers/dallas.h"
#include "drivers/ms5611.h"
#include "drivers/i2c.h"
#include "drivers/tx20.h"
#include "aprs/wx.h"
#endif

#ifdef _DALLAS_AS_TELEM
#include "drivers/dallas.h"
#endif

#include "KissCommunication.h"

//#define SERIAL_TX_TEST_MODE

// Niebieska dioda -> DCD
// Zielona dioda -> anemometr albo TX

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Wempty-body"


// global variable incremented by the SysTick handler to measure time in miliseconds
uint32_t master_time = 0;

// global variable used as a timer to trigger meteo sensors mesurements
uint32_t main_wx_sensors_pool_timer = 65500;

// global variable used as a timer to trigger packet sending
uint32_t main_packet_rx_pool_timer = 6000;

// global variables represending the AX25/APRS stack
AX25Ctx main_ax25;
Afsk main_afsk;


AX25Call main_own_path[3];
uint8_t main_own_path_ln = 0;
uint8_t main_own_aprs_msg_len;
char main_own_aprs_msg[160];

// global variable used to store return value from various functions
volatile uint8_t retval = 100;

char after_tx_lock;

unsigned short rx10m = 0, tx10m = 0, digi10m = 0, kiss10m = 0;


static void message_callback(struct AX25Msg *msg) {

}

int
main(int argc, char* argv[])
{
  // Send a greeting to the trace device (skipped on Release).
//  trace_puts("Hello ARM World!");

  int32_t ln = 0;

  RCC->APB1ENR |= (RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM7EN | RCC_APB1ENR_TIM4EN);
  RCC->APB2ENR |= (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_TIM1EN);

  memset(main_own_aprs_msg, 0x00, 128);

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

  // choosing the signal source for the SysTick timer.
  SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

  // Configuring the SysTick timer to generate interrupt 100x per second (one interrupt = 10ms)
  SysTick_Config(SystemCoreClock / SYSTICK_TICKS_PER_SECONDS);

#if defined _RANDOM_DELAY
  // configuring a default delay value
  delay_set(_DELAY_BASE, 1);
#elif !defined _RANDOM_DELAY
  delay_set(_DELAY_BASE, 0);

#endif

  // configuring an APRS path used to transmit own packets (telemetry, wx, beacons)
  main_own_path_ln = ConfigPath(main_own_path);

#ifdef _METEO
  i2cConfigure();
#endif
  LedConfig();
  AFSK_Init(&main_afsk);
  ax25_init(&main_ax25, &main_afsk, 0, 0x00);
  DA_Init();

#ifdef _METEO
  dht22_init();
  DallasInit(GPIOC, GPIO_Pin_6, GPIO_PinSource6);
  TX20Init();
#endif
#ifdef _DALLAS_AS_TELEM
  DallasInit(GPIOC, GPIO_Pin_6, GPIO_PinSource6);
#endif

  // initializing UART drvier
  srl_init();

#ifdef _METEO
 ms5611_reset(&rte_wx_ms5611_qf);
 ms5611_read_calibration(SensorCalData, &rte_wx_ms5611_qf);
 ms5611_trigger_measure(0, 0);
#endif

  // preparing initial beacon which will be sent to host PC using KISS protocol via UART
  main_own_aprs_msg_len = sprintf(main_own_aprs_msg, "=%07.2f%c%c%08.2f%c%c %s", (float)_LAT, _LATNS, _SYMBOL_F, (float)_LON, _LONWE, _SYMBOL_S, _COMMENT);

  // terminating the aprs message
  main_own_aprs_msg[main_own_aprs_msg_len] = 0;

  // 'sending' the message which will only encapsulate it inside AX25 protocol (ax25_starttx is not called here)
  ax25_sendVia(&main_ax25, main_own_path, (sizeof(main_own_path) / sizeof(*(main_own_path))), main_own_aprs_msg, main_own_aprs_msg_len);

  // SendKISSToHost function cleares the output buffer hence routine need to wait till the UART will be ready for next transmission.
  // Here this could be omitted because UART isn't used before but general idea
  while(srl_tx_state != SRL_TX_IDLE && srl_tx_state != SRL_TX_ERROR);

  // converting AX25 with beacon to KISS format
  ln = SendKISSToHost(main_afsk.tx_buf + 1, main_afsk.tx_fifo.tail - main_afsk.tx_fifo.head - 4, srl_tx_buffer, TX_BUFFER_LN);

  // checking if KISS-framing was done correctly
  if (ln != KISS_TOO_LONG_FRM) {
#ifdef SERIAL_TX_TEST_MODE
	  // infinite loop for testing UART transmission
	  for (;;) {

		  retval = srl_receive_data(100, FEND, FEND, 0, 0, 0);
#endif
		  retval = srl_start_tx(ln);

#ifdef SERIAL_TX_TEST_MODE
		  while(srl_tx_state != SRL_TX_IDLE);

		  if (srl_rx_state == SRL_RX_DONE) {
			  retval = 200;
		  }
	  }
#endif
  }

  // reinitializing AFSK and AX25 driver
  AFSK_Init(&main_afsk);

  ADCStartConfig();
  DACStartConfig();
  AFSK_Init(&main_afsk);
  ax25_init(&main_ax25, &main_afsk, 0, message_callback);

  // getting all meteo measuremenets to be sure that WX frames want be sent with zeros
  wx_get_all_measurements();

  // switching UART to receive mode to be ready for KISS frames from host
  srl_receive_data(100, FEND, FEND, 0, 0, 0);

  GPIO_ResetBits(GPIOC, GPIO_Pin_8 | GPIO_Pin_9);

  // configuting system timers
  TimerConfig();

#ifdef _BCN_ON_STARTUP
	SendOwnBeacon();
#endif

  // Infinite loop
  while (1)
    {
	  	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)) {
			telemetry_send_values(rx10m, tx10m, digi10m, kiss10m, rte_wx_temperature_dallas_valid, rte_wx_dallas_qf, rte_wx_ms5611_qf, rte_wx_dht.qf);
	  	}
	  	else {
	  		;
	  	}

	  	// if new packet has been received from radio channel
		if(new_msg_rx == 1) {
			memset(srl_tx_buffer, 0x00, sizeof(srl_tx_buffer));

			// convert message to kiss format and send it to host
			srl_start_tx(SendKISSToHost(msg.raw_data, (msg.raw_msg_len - 2), srl_tx_buffer, TX_BUFFER_LN));

			main_ax25.dcd = false;
#ifdef _DBG_TRACE
			trace_printf("APRS-RF:RadioPacketFrom=%.6s-%d,FirstPathEl=%.6s-%d\r\n", msg.src.call, msg.src.ssid, msg.rpt_lst[0].call, msg.rpt_lst[0].ssid);
#endif
#ifdef _DIGI
			// check if this packet needs to be repeated (digipeated) and do it if it is neccessary
			Digi(&msg);
#endif
			new_msg_rx = 0;
			rx10m++;
		}

		// if new KISS message has been received from the host
		if (srl_rx_state == SRL_RX_DONE) {

			// parse incoming data and then transmit on radio freq
			short res = ParseReceivedKISS(srl_get_rx_buffer(), &main_ax25, &main_afsk);
			if (res == 0)
				kiss10m++;	// increase kiss messages counter

			// restart KISS receiving to be ready for next frame
			srl_receive_data(120, FEND, FEND, 0, 0, 0);
		}

		// get all meteo measuremenets each 65 seconds. some values may not be
		// downloaded from sensors if _METEO and/or _DALLAS_AS_TELEM aren't defined
		if (main_wx_sensors_pool_timer < 10) {

			wx_get_all_measurements();

			main_wx_sensors_pool_timer = 65500;
		}

#ifdef _METEO
		// dht22 sensor communication pooling
		wx_pool_dht22();
#endif
    }
  // Infinite loop, never return.
}

uint16_t main_get_adc_sample(void) {
	return (uint16_t) ADC1->DR;
}

void main_wx_decremenet_counter(void) {
	if (main_wx_sensors_pool_timer > 0)
		main_wx_sensors_pool_timer -= SYSTICK_TICKS_PERIOD;
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
