/*
 * io.c
 *
 *  Created on: 11.06.2020
 *      Author: mateusz
 */


#include "station_config_target_hw.h"

#include "io.h"

#ifdef STM32F10X_MD_VL
#include <stm32f10x.h>
#include <drivers/f1/gpio_conf_stm32f1x.h>
#endif
#ifdef STM32L471xx
#include <stm32l4xx.h>
#include <stm32l4xx_ll_gpio.h>
#endif

#include "station_config.h"

#if defined(PARAMETEO)
LL_GPIO_InitTypeDef GPIO_InitTypeDef;

int8_t io_vbat_a_coeff, io_vbat_b_coeff;
#endif

void io_oc_init(void) {
#ifdef STM32F10X_MD_VL
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif
}

void io_oc_output_low(void) {
#ifdef STM32F10X_MD_VL

	GPIO_SetBits(GPIOA, GPIO_Pin_11);
#endif
}

void io_oc_output_hiz(void) {
#ifdef STM32F10X_MD_VL

	GPIO_ResetBits(GPIOA, GPIO_Pin_11);
#endif
}


void io_pwr_init(void) {
#if defined(STM32F10X_MD_VL)

			// RELAY_CNTRL
			GPIO_InitTypeDef GPIO_InitStructure;
			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		#if (defined PARATNC_HWREV_A || defined PARATNC_HWREV_B)
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
		#elif (defined PARATNC_HWREV_C)
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		#else
		#error ("Hardware Revision not chosen.")
		#endif
			GPIO_Init(GPIOB, &GPIO_InitStructure);

		#if (defined PARATNC_HWREV_C)
			// +12V PWR_CNTRL
			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
			GPIO_Init(GPIOA, &GPIO_InitStructure);
		#endif

			wx_pwr_state = WX_PWR_OFF;

			GPIO_ResetBits(GPIOB, GPIO_Pin_8);

		#if (defined PARATNC_HWREV_C)
			// +12V_SW PWR_CNTRL
			GPIO_ResetBits(GPIOA, GPIO_Pin_6);
		#endif

#endif

#if defined(STM32L471xx)
			// PC13 - UC_CNTRL_VS
			GPIO_InitTypeDef.Mode = LL_GPIO_MODE_OUTPUT;
			GPIO_InitTypeDef.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
			GPIO_InitTypeDef.Pin = LL_GPIO_PIN_13;
			GPIO_InitTypeDef.Pull = LL_GPIO_PULL_NO;
			GPIO_InitTypeDef.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
			GPIO_InitTypeDef.Alternate = LL_GPIO_AF_7;
			LL_GPIO_Init(GPIOC, &GPIO_InitTypeDef);

			// PA6 - UC_CNTRL_VG
			GPIO_InitTypeDef.Mode = LL_GPIO_MODE_OUTPUT;
			GPIO_InitTypeDef.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
			GPIO_InitTypeDef.Pin = LL_GPIO_PIN_6;
			GPIO_InitTypeDef.Pull = LL_GPIO_PULL_NO;
			GPIO_InitTypeDef.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
			GPIO_InitTypeDef.Alternate = LL_GPIO_AF_7;
			LL_GPIO_Init(GPIOA, &GPIO_InitTypeDef);

			// PA1 - UC_CNTRL_VC
			GPIO_InitTypeDef.Mode = LL_GPIO_MODE_OUTPUT;
			GPIO_InitTypeDef.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
			GPIO_InitTypeDef.Pin = LL_GPIO_PIN_1;
			GPIO_InitTypeDef.Pull = LL_GPIO_PULL_NO;
			GPIO_InitTypeDef.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
			GPIO_InitTypeDef.Alternate = LL_GPIO_AF_7;
			LL_GPIO_Init(GPIOA, &GPIO_InitTypeDef);

			// PB1 - UC_CNTRL_VC
			GPIO_InitTypeDef.Mode = LL_GPIO_MODE_OUTPUT;
			GPIO_InitTypeDef.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
			GPIO_InitTypeDef.Pin = LL_GPIO_PIN_1;
			GPIO_InitTypeDef.Pull = LL_GPIO_PULL_NO;
			GPIO_InitTypeDef.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
			GPIO_InitTypeDef.Alternate = LL_GPIO_AF_7;
			LL_GPIO_Init(GPIOB, &GPIO_InitTypeDef);

#endif
}


void io_ext_watchdog_config(void) {
#ifdef STM32F10X_MD_VL
	  // initialize Watchdog output
	  Configure_GPIO(GPIOA,12,GPPP_OUTPUT_50MHZ);
#endif

}

void io_ext_watchdog_service(void) {
#ifdef STM32F10X_MD_VL

#endif

#ifdef STM32L471xx
	if ((GPIOA->ODR & GPIO_ODR_OD1) == 0) {
		// set high
		GPIOA->BSRR |= GPIO_BSRR_BS1;
	}
	else {
		// set low
		GPIOA->BSRR |= GPIO_BSRR_BR1;
	}
#endif
}

void io_vbat_meas_init(int8_t a_coeff, int8_t b_coeff) {
	io_vbat_a_coeff = a_coeff;
	io_vbat_b_coeff = b_coeff;

	GPIO_InitTypeDef.Mode = LL_GPIO_MODE_ANALOG;
	GPIO_InitTypeDef.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitTypeDef.Pin = LL_GPIO_PIN_5;
	GPIO_InitTypeDef.Pull = LL_GPIO_PULL_NO;
	GPIO_InitTypeDef.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
	GPIO_InitTypeDef.Alternate = LL_GPIO_AF_7;
	LL_GPIO_Init(GPIOC, &GPIO_InitTypeDef);

	LL_GPIO_EnablePinAnalogControl(GPIOC, LL_GPIO_PIN_5);


	volatile int stupid_delay = 0;

	// reset the clock for ADC
//	RCC->AHB2ENR &= (0xFFFFFFFF ^ RCC_AHB2ENR_ADCEN);
//	RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;

	// the adc should be disabled now, but just to be sure that this is a case
	ADC2->CR &= (0xFFFFFFFF ^ ADC_CR_ADEN);

	// exit from deep-power-down mode
	ADC2->CR &= (0xFFFFFFFF ^ ADC_CR_DEEPPWD);

	// start ADC voltage regulator
	ADC2->CR |= ADC_CR_ADVREGEN;

	// wait for voltage regulator to start
	for (; stupid_delay < 0x1FFFF; stupid_delay++);

	// start the calibration
	ADC2->CR |= ADC_CR_ADCAL;

	// wait for calibration to finish
    while((ADC2->CR & ADC_CR_ADCAL) == ADC_CR_ADCAL);

    // set the first (and only channel in a conversion sequence) channel 14
    ADC2->SQR1 |= (14 << 6);

    // set the sampling rate to 12.5 ADC clock cycles
    ADC2->SMPR1 |= 0x2;

	ADC2->CFGR &= (0xFFFFFFFF ^ ADC_CFGR_CONT);

    // set discontinous conversion
	ADC2->CFGR |= ADC_CFGR_DISCEN;

	// ignore overrun and overwrite data register content with new conversion result
	ADC2->CFGR |= ADC_CFGR_OVRMOD;


}

uint16_t io_vbat_meas_get(void) {

	uint16_t out = 0;

	float temp = 0.0f;

	// start ADC
	ADC2->CR |= ADC_CR_ADEN;

	// wait for startup
    while((ADC2->ISR & ADC_ISR_ADRDY) == 0);

	// start conversion
	ADC2->CR |= ADC_CR_ADSTART;

	// wait for conversion to finish
    while((ADC2->ISR & ADC_ISR_EOC) != ADC_ISR_EOC) {
    	;
    }

	out = ADC2->DR;

	// disable ADC
	ADC2->CR &= (0xFFFFFFFF ^ ADC_CR_ADEN);

	temp = (float)out * 0.00081f;

	out = (uint16_t) (temp * (float)io_vbat_a_coeff + (float)io_vbat_b_coeff);

	return out;
}

