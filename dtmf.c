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

#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "dtmf.h"

// Samples table: one period sampled on 128 samples and quantized on 7 bit
const uint8_t auc_sin_param[SAMPLE_SIZE] = {
    64,  67,  70,  73,  76,  79,  82,  85,
    88,  91,  94,  96,  99,  102, 104, 106,
    109, 111, 113, 115, 117, 118, 120, 121,
    123, 124, 125, 126, 126, 127, 127, 127,
    127, 127, 127, 127, 126, 126, 125, 124,
    123, 121, 120, 118, 117, 115, 113, 111,
    109, 106, 104, 102, 99,  96,  94,  91,
    88,  85,  82,  79,  76,  73,  70,  67,
    64,  60,  57,  54,  51,  48,  45,  42,
    39,  36,  33,  31,  28,  25,  23,  21,
    18,  16,  14,  12,  10,  9,   7,   6,
    4,   3,   2,   1,   1,   0,   0,   0,
    0,   0,   0,   0,   1,   1,   2,   3,
    4,   6,   7,   9,   10,  12,  14,  16,
    18,  21,  23,  25,  28,  31,  33,  36,
    39,  42,  45,  48,  51,  54,  57,  60
};

//  L\H | 1209 | 1336 | 1477
//  697 |   1  |   2  |   3
//  770 |   4  |   5  |   6
//  852 |   7  |   8  |   9
//  941 |   *  |   0  |   #

const uint8_t auc_frequency[][2] =
{
    { FREQ_H2, FREQ_L4 }, // 0
    { FREQ_H1, FREQ_L1 }, // 1
    { FREQ_H2, FREQ_L1 }, // 2
    { FREQ_H3, FREQ_L1 }, // 3
    { FREQ_H1, FREQ_L2 }, // 4
    { FREQ_H2, FREQ_L2 }, // 5
    { FREQ_H3, FREQ_L2 }, // 6
    { FREQ_H1, FREQ_L3 }, // 7
    { FREQ_H2, FREQ_L3 }, // 8
    { FREQ_H3, FREQ_L3 }, // 9
    { FREQ_H1, FREQ_L4 }, // *
    { FREQ_H3, FREQ_L4 }, // #
};

volatile uint32_t _g_delay_counter;   // Delay counter for sleep function
volatile uint8_t _g_stepwidth_a;      // step width of high frequency
volatile uint8_t _g_stepwidth_b;      // step width of low frequency
volatile uint16_t _g_cur_sin_val_a;   // position freq. A in LUT (extended format)
volatile uint16_t _g_cur_sin_val_b;   // position freq. B in LUT (extended format)

void dtmf_init()
{
    TIMSK = _BV(TOIE0);               // Int T0 Overflow enabled
    TCCR0A = _BV(WGM00) | _BV(WGM01); // 8Bit PWM; Compare/match output mode configured later
    TCCR0B = _BV(CS00);               // F_CPU no prescaler
    TCNT0 = 0;
    OCR0A = 0;
    DDRB |= _BV(PIN_PWM_OUT);         // PWM output (OC0A pin)

    _g_delay_counter = 0;
    _g_stepwidth_a = _g_stepwidth_b = 0;
    _g_cur_sin_val_a = _g_cur_sin_val_b = 0;
}

// Generate DTMF tone, duration x ms
void dtmf_generate_tone(int8_t digit, uint16_t duration_ms)
{
    GIMSK = 0;

    if (IS_DIGIT(digit))
    {
        // Standard digits 0-9, *, #
        _g_stepwidth_a = auc_frequency[digit][0];
        _g_stepwidth_b = auc_frequency[digit][1];
        dtmf_enable_pwm();

        // Wait x ms
        sleep_ms(duration_ms);
    }
    else if (digit == DIGIT_BEEP)
        // Beep ~1000Hz
        play_tone(FREQ_BEEP, duration_ms);
    else if (digit == DIGIT_BEEP_LOW)
        // Beep ~500Hz
        play_tone(FREQ_BEEP_LOW, duration_ms);
    else if (digit == DIGIT_TUNE_ASC)
        play_tune(FREQ_DO, FREQ_MI, FREQ_SOL, duration_ms);
    else if (digit == DIGIT_TUNE_DESC)
        play_tune(FREQ_SOL, FREQ_MI, FREQ_DO, duration_ms);

    // Stop DTMF transmitting
    dtmf_disable_pwm();

    _g_stepwidth_a = 0;
    _g_stepwidth_b = 0;

    GIMSK = _BV(INT0) | _BV(PCIE);
}

void play_tone(uint8_t freq, uint16_t duration_ms)
{
    _g_stepwidth_a = freq;
    _g_stepwidth_b = 0;
    dtmf_enable_pwm();
    sleep_ms(duration_ms);
}

void play_tune(uint8_t f1, uint8_t f2, uint8_t f3, uint16_t duration_ms)
{
    duration_ms /= 3;
    _g_stepwidth_a = f1;
    _g_stepwidth_b = 0;
    dtmf_enable_pwm();
    sleep_ms(duration_ms);
    _g_stepwidth_a = f2;
    sleep_ms(duration_ms);
    _g_stepwidth_a = f3;
    sleep_ms(duration_ms);
}

// Enable PWM output by configuring compare match mode - non inverting PWM
void dtmf_enable_pwm()
{
    TCCR0A |= _BV(COM0A1);
    TCCR0A &= ~_BV(COM0A0);
}

// Disable PWM output (compare match mode 0) and force it to 0
void dtmf_disable_pwm()
{
    TCCR0A &= ~_BV(COM0A1);
    TCCR0A &= ~_BV(COM0A0);
    PORTB &= ~_BV(PIN_PWM_OUT);
}

// Timer overflow interrupt service routine
ISR(TIMER0_OVF_vect)
{
    uint8_t sin_a, sin_b = 0;

    // A component (high frequency) is always used
    sin_a = auc_sin_param[PHASE_JUMP(_g_cur_sin_val_a += _g_stepwidth_a)];

    // B component (low frequency) is optional
    if (_g_stepwidth_b > 0)
        sin_b = auc_sin_param[PHASE_JUMP(_g_cur_sin_val_b += _g_stepwidth_b)];
    else
        // double A component
        sin_a <<= 1;

    // calculate PWM value: high frequency value + 3/4 low frequency value
    OCR0A = (sin_a + (sin_b - (sin_b >> 2)));
    _g_delay_counter++;
}

// Wait x ms
void sleep_ms(uint16_t msec)
{
    _g_delay_counter = 0;
    set_sleep_mode(SLEEP_MODE_IDLE);
    uint16_t ticks = msec * T0_OVERFLOW_PER_S / 1000;
    while(_g_delay_counter <= ticks)
        sleep_mode();
}
