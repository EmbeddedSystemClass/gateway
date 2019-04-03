/******************************************************************************
* File Name          : adcparamsinit.h
* Date First Issued  : 03/09/2019
* Board              : DiscoveryF4
* Description        : Initialization of parameters for ADC app configuration
*******************************************************************************/

#ifndef __ADCPARAMSINIT
#define __ADCPARAMSINIT

#include <stdint.h>
#include "adcparams.h"

#define SCALE1 (1 << 16)

/* Factory calibration pointers. */
#define PVREFINT_CAL ((uint16_t*)0x1FFF7A2A)  // Pointer to factory calibration: Vref
#define PTS_CAL1     ((uint16_t*)0x1FFF7A2C)  // Pointer to factory calibration: Vtemp
#define PTS_CAL2     ((uint16_t*)0x1FFF7A2E)  // Pointer to factory calibration: Vtemp

/* Factory Vdd for Vref calibration. */
#define VREFCALVOLT 3300  // Factory cal voltage (mv)
#define VREFCALVOLTF (VREFCALVOLT * 0.001)  // Factory cal voltage, float (volts)

/* *************************************************************************/
void adcparamsinit_init_common(struct ADCCALCOMMON* padccommon);
/*	@brief	: Initialize struct with parameters common to all ADC for this board
 * @param	: padccommon = pointer to struct 
 * *************************************************************************/
void adcparamsinit_init(struct ADCCHANNELSTUFF* pacsx);
/*	@brief	: Load structs for compensation, calibration and filtering all ADC channels
 * @param	: pacsx = Pointer to struct "everything" for this ADC module
 * *************************************************************************/

#endif

