/******************************************************************************
* File Name          : adcparamsinit.c
* Date First Issued  : 03/09/2019
* Board              : DiscoveryF4
* Description        : Initialization of parameters for ADC app configuration
*******************************************************************************/

/* 
This is where hard-coded parameters for the ADC are entered.

Later, this may be replaced with a "copy" of the flat file in high flash, generated
by the java program from the sql database.
*/

#include "adcparamsinit.h"
#include "adcparams.h"

/* *************************************************************************
 * void adcparamsinit_init_common(struct ADCCALCOMMON* padccommon);
 *	@brief	: Initialize struct with parameters common to all ADC for this =>board<=
 * @param	: padccommon = pointer to struct holding parameters
 * *************************************************************************/
void adcparamsinit_init_common(struct ADCCALCOMMON* padccommon)
{

	padccommon->sensor5vcal = 0.54 / ADCSEQNUM;	// 5v->Vdd divide ratio
	padccommon->sensor5vcalVdd = padccommon->sensor5vcal / 3.3; // Precompute: adjust for Vdd

	padccommon->ts_vref = *PVREFINT_CAL; // Factory calibration
	padccommon->tcoef   = 30E-6; // 30 typ, 50 max, (ppm/deg C)

	padccommon->ts_cal1      = (float)(*PTS_CAL1) * (float)ADC1DMANUMSEQ; // Factory calibration
	padccommon->ts_cal2      = *PTS_CAL2; // Factory calibration
	padccommon->ts_caldiff   = *PTS_CAL2 - *PTS_CAL1; // Pre-compute
	padccommon->ts_80caldiff = (80.0 / (padccommon->ts_caldiff *(float)ADC1DMANUMSEQ)); // Pre-compute

	padccommon->uicaldiff    = *PTS_CAL2 - *PTS_CAL1; // Pre-compute
	padccommon->ll_80caldiff = (80 * SCALE1) /(padccommon->uicaldiff);
	padccommon->ui_cal1      =	(*PTS_CAL1) * ADC1DMANUMSEQ;

	/* Data sheet gave these values.  May not need them. */
	padccommon->v25     = 0.76; // Voltage at 25 °C, typ
	padccommon->slope   = 2.0;  // Average slope (mv/deg C), typ

	return;
}

/* *************************************************************************
 * void adcparamsinit_init(struct ADCCHANNELSTUFF* pacsx);
 *	@brief	: Load structs for compensation, calibration and filtering for ADC channels
 * @param	: pacsx = Pointer to struct "everything" for this ADC module
 * *************************************************************************/
void adcparamsinit_init(struct ADCCHANNELSTUFF* pacsx)
{
	struct ADCCHANNELSTUFF* pacs; // Use pointer for convenience

/* IN18 - Internal voltage reference */
	pacs = pacsx + ADC1IDX_INTERNALVREF; // Point to "stuff" for this ADC channel

	// Filter type, calibration option, compensation option. */
	pacs->xprms.filttype  = ADCFILTERTYPE_IIR1;      // Single pole IIR
	pacs->xprms.calibtype = ADC1PARAM_CALIBTYPE_RAW; // Raw; no calibration applied
	pacs->xprms.comptype  = ADC1PARAM_COMPTYPE_NONE; // No temperature compenstaion

	// Calibration coefficients.
	pacs->cal.f[0] = 0.0;  // Offset
	pacs->cal.f[1] = 1.0;  // Scale (jic calibration not skipped)

	// Filter initialize, coefficients, and pre-computed value. */
	pacs->fpw.iir_f1.skipctr  = 2500; 	 // Initial readings skip count
	pacs->fpw.iir_f1.coef     = 0.999;  // Filter coefficient (< 1.0)
	pacs->fpw.iir_f1.onemcoef = (1 - pacs->fpw.iir_f1.coef);

/* IN17 - Internal temperature sensor */
	pacs = pacsx + ADC1IDX_INTERNALTEMP; // Point to "stuff" for this ADC channel

	// Filter type, calibration option, compensation option. */
	pacs->xprms.filttype  = ADCFILTERTYPE_IIR1;      // Single pole IIR
	pacs->xprms.calibtype = ADC1PARAM_CALIBTYPE_RAW; // Raw; no calibration applied
	pacs->xprms.comptype  = ADC1PARAM_COMPTYPE_NONE; // No temperature compenstaion

	// Calibration coefficients.
	pacs->cal.f[0] = 0.0;  // Offset
	pacs->cal.f[1] = 1.0;  // Scale (jic calibration not skipped)

	// Filter initialize, coefficients, and pre-computed value. */
	pacs->fpw.iir_f1.skipctr  = 4; 	 // Initial readings skip count
	pacs->fpw.iir_f1.coef     = 0.9;  // Filter coefficient (< 1.0)
	pacs->fpw.iir_f1.onemcoef = (1 - pacs->fpw.iir_f1.coef);

/* Hall effect lever.  5v supply. */
	pacs = pacsx + ADC1IDX_HALLLEVER; // Point to "stuff" for this ADC channel

	// Filter type, calibration option, compensation option. */
	pacs->xprms.filttype  = ADCFILTERTYPE_IIR1;        // Single pole IIR
	pacs->xprms.calibtype = ADC1PARAM_CALIBTYPE_OFSC;  // Offset & scale (poly ord 0 & 1)
	pacs->xprms.comptype  = ADC1PARAM_COMPTYPE_VOLT5A; // 5v sensor; Vref w 5v supply reading compensation

	// Calibration coefficients.
	pacs->cal.f[0] = 0.0;     // Offset
	pacs->cal.f[1] = 0.5465; // Scale (volts) (3.9K - 4.7K)

	// Filter initialize, coefficients, and pre-computed value. */
	pacs->fpw.iir_f1.skipctr  = 4; 	 // Initial readings skip count
	pacs->fpw.iir_f1.coef     = 0.9;   // Filter coefficient (< 1.0)
	pacs->fpw.iir_f1.onemcoef = (1 - pacs->fpw.iir_f1.coef);

/* Resistor pot connected to 3.3v (Vdd) supply. */
	pacs = pacsx + ADC1IDX_RESISRPOT; // Point to "stuff" for this ADC channel

	// Filter type, calibration option, compensation option. */
	pacs->xprms.filttype  = ADCFILTERTYPE_IIR1;        // Single pole IIR
	pacs->xprms.calibtype = ADC1PARAM_CALIBTYPE_OFSC;  // Offset & scale (poly ord 0 & 1)
	pacs->xprms.comptype  = ADC1PARAM_COMPTYPE_VOLT3AT; // 5v sensor; Vref abs w temp

	// Calibration coefficients.
	pacs->cal.f[0] = 0.0;           // Offset
	pacs->cal.f[1] = (100.0/4095); // Scale

	// Filter initialize, coefficients, and pre-computed value. */
	pacs->fpw.iir_f1.skipctr  = 4; 	 // Initial readings skip count
	pacs->fpw.iir_f1.coef     = 0.9;   // Filter coefficient (< 1.0)
	pacs->fpw.iir_f1.onemcoef = (1 - pacs->fpw.iir_f1.coef);

/* Total battery current sensor. */
	pacs = pacsx + ADC1IDX_CURRENTTOTAL; // Point to "stuff" for this ADC channel

	// Filter type, calibration option, compensation option. */
	pacs->xprms.filttype  = ADCFILTERTYPE_IIR1;        // Single pole IIR
	pacs->xprms.calibtype = ADC1PARAM_CALIBTYPE_OFSC;  // Offset & scale (poly ord 0 & 1)
	pacs->xprms.comptype  = ADC1PARAM_COMPTYPE_VOLT5AT; // 5v w Vref abs w temp

	// Calibration coefficients.
	pacs->cal.f[0] = 2047.5; // Offset
	pacs->cal.f[1] = 0.1086; // Scale (200a @saturation)

	// Filter initialize, coefficients, and pre-computed value. */
	pacs->fpw.iir_f1.skipctr  = 4; 	 // Initial readings skip count
	pacs->fpw.iir_f1.coef     = 0.9;   // Filter coefficient (< 1.0)
	pacs->fpw.iir_f1.onemcoef = (1 - pacs->fpw.iir_f1.coef);

/* Current sensor: motor #1 */
	pacs = pacsx + ADC1IDX_CURRENTMOTOR1; // Point to "stuff" for this ADC channel

	// Filter type, calibration option, compensation option. */
	pacs->xprms.filttype  = ADCFILTERTYPE_IIR1;        // Single pole IIR
	pacs->xprms.calibtype = ADC1PARAM_CALIBTYPE_OFSC;  // Offset & scale (poly ord 0 & 1)
	pacs->xprms.comptype  = ADC1PARAM_COMPTYPE_VOLT5AT; // 5v w Vref abs w temp

	// Calibration coefficients.
	pacs->cal.f[0] = 2047.5;  // Offset
	pacs->cal.f[1] = 0.3257;  // Scale (600a @saturation)

	// Filter initialize, coefficients, and pre-computed value. */
	pacs->fpw.iir_f1.skipctr  = 4; 	 // Initial readings skip count
	pacs->fpw.iir_f1.coef     = 0.9;   // Filter coefficient (< 1.0)
	pacs->fpw.iir_f1.onemcoef = (1 - pacs->fpw.iir_f1.coef);

/* Current sensor: motor #2 */
	pacs = pacsx + ADC1IDX_CURRENTMOTOR2; // Point to "stuff" for this ADC channel

	// Filter type, calibration option, compensation option. */
	pacs->xprms.filttype  = ADCFILTERTYPE_IIR1;        // Single pole IIR
	pacs->xprms.calibtype = ADC1PARAM_CALIBTYPE_OFSC;  // Offset & scale (poly ord 0 & 1)
	pacs->xprms.comptype  = ADC1PARAM_COMPTYPE_VOLT5AT; // 5v w Vref abs w temp

	// Calibration coefficients.
	pacs->cal.f[0] = 2047.5;  // Offset
	pacs->cal.f[1] = 0.2172;  // Scale (400a @saturation)

	// Filter initialize, coefficients, and pre-computed value. */
	pacs->fpw.iir_f1.skipctr  = 4; 	 // Initial readings skip count
	pacs->fpw.iir_f1.coef     = 0.9;   // Filter coefficient (< 1.0)
	pacs->fpw.iir_f1.onemcoef = (1 - pacs->fpw.iir_f1.coef);

/* +12v supply voltage */
	pacs = pacsx + ADC1IDX_12VRAWSUPPLY; // Point to "stuff" for this ADC channel

	// Filter type, calibration option, compensation option. */
	pacs->xprms.filttype  = ADCFILTERTYPE_IIR1;        // Single pole IIR
	pacs->xprms.calibtype = ADC1PARAM_CALIBTYPE_OFSC;  // Offset & scale (poly ord 0 & 1)
	pacs->xprms.comptype  = ADC1PARAM_COMPTYPE_VOLT5AT; // 5v w Vref abs w temp

	// Calibration coefficients.
	pacs->cal.f[0] = 0.0;     // Offset
	pacs->cal.f[1] = 0.1525; // Scale (volts) (1.8K-10K)

	// Filter initialize, coefficients, and pre-computed value. */
	pacs->fpw.iir_f1.skipctr  = 4; 	 // Initial readings skip count
	pacs->fpw.iir_f1.coef     = 0.9;   // Filter coefficient (< 1.0)
	pacs->fpw.iir_f1.onemcoef = (1 - pacs->fpw.iir_f1.coef);


/* 5v supply. */
	pacs = pacsx + ADC1IDX_5VOLTSUPPLY; // Point to "stuff" for this ADC channel

	// Filter type, calibration option, compensation option. */
	pacs->xprms.filttype  = ADCFILTERTYPE_IIR1;        // Single pole IIR
	pacs->xprms.calibtype = ADC1PARAM_CALIBTYPE_OFSC;  // Offset & scale (poly ord 0 & 1)
	pacs->xprms.comptype  = ADC1PARAM_COMPTYPE_VOLT5A; // 5v sensor; Vref w 5v supply reading compensation

	// Calibration coefficients.
	pacs->cal.f[0] = 0.0;    // Offset
	pacs->cal.f[1] = 0.5465; // Scale (volts) (3.9K - 4.7K)

	// Filter initialize, coefficients, and pre-computed value. */
	pacs->fpw.iir_f1.skipctr  = 4; 	 // Initial readings skip count
	pacs->fpw.iir_f1.coef     = 0.9;   // Filter coefficient (< 1.0)
	pacs->fpw.iir_f1.onemcoef = (1 - pacs->fpw.iir_f1.coef);

	return;
};
