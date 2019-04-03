/******************************************************************************
* File Name          : adcparams.c
* Date First Issued  : 03/09/2019
* Board              : DiscoveryF4
* Description        : Parameters for ADC app configuration
*******************************************************************************/
/*
Not thread safe.
*/
#include "adcparams.h"
#include "adcparamsinit.h"

#include "DTW_counter.h"



/*
AN3964
https://www.st.com/resource/en/application_note/dm00035957.pdf
V DDA = 3 × Val_V REFINT_CAL ⁄ Val_V REFINT

Temp = 80 ⁄ ( TS_CAL2 – TS_CAL1 ) × ( ValTS – TS_CAL1 ) + 30

The accurate embedded internal reference voltage (V REFINT ) is individually sampled by the
ADC, and the converted value for each device (Val_V REFINT_CAL ) is stored during the
manufacturing process in the protected memory area at address VREFINT_CAL specified
in the product datasheet. The internal reference voltage calibration data is a 12-bit unsigned
number (right-aligned bits, stored in 2 bytes) acquired by the STM32L1x ADC referenced to
V VREF_MEAS = V REF+ = 3V ± 0.01V
The total accuracy of the factory measured calibration data is then provided with an
accuracy of ± 5 mV (refer to the datasheet for more details).
We can determine the actual V DDA voltage by using the formula above as follows:
V DDA = 3 × Val_V REFINT_CAL ⁄ Val_V REFINT
The temperature sensor data, ValTS_bat, are sampled with the ADC scale referenced to the
actual V DDA value determined at the previous steps. Since the temperature sensor factory
calibration data are acquired with the ADC scale set to 3 V, we need to normalize ValTS_bat
to get the temperature sensor data (ValTS) as it would be acquired with ADC scale set to
3 V. ValTS_bat can be normalized by using the formula below:
ValTS = 3 × ValTS_bat ⁄ V DDA
If the ADC is referenced to the 3 V power supply (which is the case of the STM32L1
Discovery) such a normalization is not needed and the sampled temperature data can be
directly used to determine the temperature as described in Section 2.2.1: Temperature
sensor calibration.


Vdd = 3300*(*VREFINT_CAL_ADDR)/ADC_raw;

Temp(degree) = (V_sense - V_25)/Avg_slope + 25

*/

#define VREFINT_CAL_ADDR 

/* Calibration values common to all ADC modules. */
struct ADCCALCOMMON adcommon;

/* ADC1 parameters, calibrations, filtering, ... */
struct ADCCHANNELSTUFF adc1channelstuff[ADC1IDX_ADCSCANSIZE];

/* Raw and calibrated ADC1 readings. */
struct ADC1DATA adc1data;

/* *************************************************************************
 * void adcparams_init(void);
 *	@brief	: Copy parameters into structs
 * NOTE: => ASSUMES ADC1 ONLY <==
 * *************************************************************************/
void adcparams_init(void)
{
	/* Common to board */
	adcparamsinit_init_common(&adcommon);

	/* Load parameter values for ADC channels. */
	adcparamsinit_init(adc1channelstuff);
	return;
}

/* *************************************************************************
 * void adcparams_internal(struct ADCCALCOMMON* pacom, uint16_t* ptemp, uint316_t* pvref);
 *	@brief	: Update values used for compensation from Vref and Temperature
 * @param	: pacom = Pointer calibration parameters for Temperature and Vref
 * @param	: ptemp = Pointer to summed DMA reading
 * @param	: pvref = Pointer to summed Vref reading
 * *************************************************************************/
uint32_t adcdbg1;
uint32_t adcdbg2;
void adcparams_internal(struct ADCCALCOMMON* pacom, uint16_t* ptemp, uint16_t* pvref)
{
/* 
   Reproduced from 'adcparamsinit.h' for convenience.
#define PVREFINT_CAL ((uint16_t*)0x1FFF7A2A))  // Pointer to factory calibration: Vref
#define PTS_CAL1     ((uint16_t*)0x1FFF7A2C))  // Pointer to factory calibration: Vtemp
#define PTS_CAL2     ((uint16_t*)0x1FFF7A2E))  // Pointer to factory calibration: Vtemp
*/

/* The following two computations with floats use 1500 machines cycles. */
	/* Vdd computed from Vrefint using factory calibration. */
//	pacom->fvdd  = (3.300 * (float)ADC1DMANUMSEQ * (*PVREFINT_CAL)) /  (float)(*pvref);
	
	/* Temperature computed from internal sensor using factory 
      calibrations @ Vdd = 3.3v, and adjusted for measured Vdd. */
//	pacom->degC = (pacom->ts_80caldiff) * ( (float)(*ptemp) * ( ( pacom->fvdd * (1.0/3.3) ) ) - pacom->ts_cal1)  + 30;

adcdbg1 = DTWTIME;
	
/* The following two computaions with ints uses 119 machines cycles. */
	pacom->ivdd = (3300 * ADC1DMANUMSEQ) * (*PVREFINT_CAL) / (*pvref);

	pacom->ui_tmp = (pacom->ivdd * (*ptemp) ) / 3300; // Adjust for Vdd not at 3.3v calibration
	pacom->degC  = pacom->ll_80caldiff * (pacom->ui_tmp - pacom->ui_cal1) + (30 * SCALE1 * ADC1DMANUMSEQ);
	pacom->degC *= (1.0/(SCALE1*ADC1DMANUMSEQ)); // Fast because power of two.

	pacom->fvdd = pacom->ivdd;
	pacom->fvdd = pacom->fvdd + pacom->tcoef * (pacom->degC - 30);

	pacom->fvddfilt = iir_f1_f(&adc1channelstuff[ADC1IDX_INTERNALVREF].fpw.iir_f1, pacom->fvdd);

	pacom->fvddcomp = pacom->fvddfilt * pacom->sensor5vcalVdd;

	pacom->fvddrecip = 1.0/pacom->fvddfilt

	/* Scale up for fixed division, then convert to float and descale. */
	pacom->f5_Vddratio = ( (adc1data.adcs1sum[ADC1IDX_INTERNALVREF] * (1<<12)) /
       adc1data.adcs1sum[ADC1IDX_5VOLTSUPPLY]);
	pacom->f5_Vddratio *= (1.0/(1<<12));


adcdbg2 = DTWTIME - adcdbg1;

	return;
}
* *************************************************************************
 * void adcparams_chan(uint8_t adcidx);
 *	@brief	: calibration, compensation, filtering for channels
 * @param	: adcidx = index into ADC1 array
 * *************************************************************************/
void adcparams_internal(uint8_t adcidx)
{
	struct ADCCHANNELSTUFF* pstuff = &adc1channelstuff[adcidx];
	struct ADC1DATA* pdata         = &adc1data;
	union ADCCALREADING* pread     = &adc1calreading[adcidx];
	uint16_t* psum16               = pdata->adcs1sum[adcidx];
	struct ADCCALCOMMON* pacom     = &adcommon;


	/* Compensation type */
/* Assumes 5v sensor supply is measured with an ADC channel.
#define ADC1PARAM_COMPTYPE_NONE      0     // No supply or temp compensation applied
#define ADC1PARAM_COMPTYPE_RATIOVDD  1     // Vdd (3.3v nominal) ratiometric
#define ADC1PARAM_COMPTYPE_RATIO5V   2     // 5v ratiometric with 5->Vdd measurement
#define ADC1PARAM_COMPTYPE_RATIO5VNO 3     // 5v ratiometric without 5->Vdd measurement
#define ADC1PARAM_COMPTYPE_VOLTVDD   4     // Vdd (absolute), Vref compensation applied
#define ADC1PARAM_COMPTYPE_VOLTVDDNO 5     // Vdd (absolute), no Vref compensation applied
#define ADC1PARAM_COMPTYPE_VOLTV5    6     // 5v (absolute), with 5->Vdd measurement applied
#define ADC1PARAM_COMPTYPE_VOLTV5NO  7     // 5v (absolute), without 5->Vdd measurement applied

*/
	if (pstuff->xprms.filttype == ADC1PARAM_CALIBTYPE_RAW_UI)
	{
		pread->ui = pdata->adcs1sum[acidx]; // adc sum as unsigned int
	}
	else
	{
		pread->f = pdata->adcs1sum[acidx]; // Convert adc sum to float
	}

	switch(pstuff->xprms.comptype)
	{
	case ADC1PARAM_COMPTYPE_NONE:      // 0 No supply or temp compensation applied
		break;

	case ADC1PARAM_COMPTYPE_RATIOVDD:  // 1 Vdd (3.3v nominal) ratiometric
		pread->f *= pread->f * (100.0/(4095.0 * ADCSEQNUM))  ; // ratio: 0 - 100
		break;

	case ADC1PARAM_COMPTYPE_RATIO5V:   // 2 5v ratiometric with 5->Vdd measurement	
		pread->f *= pacom->fvddcomp;
		break;

	case ADC1PARAM_COMPTYPE_RATIO5VNO: // 3 5v ratiometric without 5->Vdd measurement
		pread->f *= pacom->sensor5vcal;
		break;

	case ADC1PARAM_COMPTYPE_VOLTVDD:   // 4 Vdd (absolute), Vref compensation applied
		pread->f *= 
		break;

	case ADC1PARAM_COMPTYPE_VOLTVDDNO: // 5 Vdd (absolute), no Vref compensation applied
		pread->f *= pacom->fvddfilt * (1.0/3.3);
		break;

	case ADC1PARAM_COMPTYPE_VOLTV5:    // 6 5v (absolute), with 5->Vdd measurement applied	
		pread->f *=
		break;	
	case ADC1PARAM_COMPTYPE_VOLTV5NO:  // 7 5v (absolute), without 5->Vdd measurement applied
		pread->f *= pacom->sensor5vcal * (1.0/ADCSEQNUM)
		break;

	default:
		return;
	}

}
