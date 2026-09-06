//*****************************************************************************
// Title        : Pulse to tone (DTMF) converter
// Author       : Boris Cherkasskiy
//                http://boris0.blogspot.ca/2013/09/rotary-dial-for-digital-age.html
// Created      : 2011-10-24
//
// Modified     : Arnie Weber 2015-06-22
//                https://bitbucket.org/310weber/rotary_dial/
//                NOTE: This code is not compatible with Boris's original hardware
//                due to changed pin-out (see Eagle files for details)
//
// Modified     : Matthew Millman 2018-05-29
//                http://tech.mattmillman.com/
//                Cleaned up implementation, modified to work more like the
//                Rotatone product.
//
// Modified     : Cesare Giannetti 2026-09-06
//                https://github.com/cesaregiannetti/rotarydial
//                Parametric frequencies allow to use 8MHz crystal
//
// This code is distributed under the GNU Public License
// which can be found at http://www.gnu.org/licenses/gpl.txt
//
// DTMF generator logic is loosely based on the AVR314 app note from Atmel
//
//*****************************************************************************

#ifndef __DTMF_H__
#define __DTMF_H__

#define DIGIT_BEEP          -10
#define DIGIT_BEEP_LOW      -13
#define DIGIT_TUNE_ASC      -11
#define DIGIT_TUNE_DESC     -12
#define DIGIT_OFF           -1
#define DIGIT_STAR          10
#define DIGIT_POUND         11

#define DTMF_DURATION_MS    100

#define FAST_PWM_PERIOD     256 // ATTYNY85 specs
#define T0_OVERFLOW_PER_S   (F_CPU / FAST_PWM_PERIOD)
#define T0_OVERFLOW_PER_MS  (int)(T0_OVERFLOW_PER_S / 1000)

#define PIN_PWM_OUT         PB0     // PB0 (OC0A) as PWM output

// Sine table sample len
#define SAMPLE_BITS         7
#define NUM_SAMPLES         _BV(SAMPLE_BITS)
#define SAMPLE_MASK         (NUM_SAMPLES - 1)

// Sine phase resolution
#define PHASE_BITS          3
#define PHASE_HALF          _BV(PHASE_BITS - 1)
#define PHASE_SIZE          _BV(PHASE_BITS + SAMPLE_BITS)

// Sine phase projection
#define PHASE_STEP(F)       ((long)(F) * PHASE_SIZE / T0_OVERFLOW_PER_S)
#define PHASE_JUMP(P)       ((((P) + PHASE_HALF) >> PHASE_BITS) & SAMPLE_MASK)

#define FREQ_H1             PHASE_STEP(1209)
#define FREQ_H2             PHASE_STEP(1336)
#define FREQ_H3             PHASE_STEP(1477)
#define FREQ_L1             PHASE_STEP(697)
#define FREQ_L2             PHASE_STEP(770)
#define FREQ_L3             PHASE_STEP(852)
#define FREQ_L4             PHASE_STEP(941)
#define FREQ_BEEP_LOW       PHASE_STEP(500)
#define FREQ_C              PHASE_STEP(523)
#define FREQ_E              PHASE_STEP(659)
#define FREQ_G              PHASE_STEP(784)
#define FREQ_BEEP           PHASE_STEP(1000)

void dtmf_init(void);
void dtmf_generate_tone(int8_t digit, uint16_t duration_ms);
void sleep_ms(uint16_t msec);

extern volatile uint32_t _g_delay_counter;

#endif /* __DTMF_H__ */


