#ifndef PWM_CONTROLLER_H
#define PWM_CONTROLLER_H

#include <math.h>

#include "LUT.h"

// --- 基本パラメータ設定 ---
#define F_CPU 16000000UL
#define PRESCALER_VALUE 8
#define TIMER_CLOCK (F_CPU / PRESCALER_VALUE)

// 位相倍率(2^n)
#define SHIFT 2097152

// 本番のPWM生成に使うためのちゃんと整形されたPWM設定。
typedef struct pwm_config {
    volatile float carrier_freq_hz;
    volatile float signal_freq_hz;
    volatile float modulation_index;  // 信号波振幅
    volatile uint8_t sig_mode;
    volatile uint16_t top;
    volatile uint32_t increment;
} pwm_config;

// PWMパターン設定から送られてくるナマのPWM設定
typedef struct PulseModeReference {
    volatile float fCarrier;
    volatile float fSig;
    volatile float mVoltage;
    volatile bool ThiEnable;
    volatile bool SvmEnable;
} PulseModeReference;

// 電圧補償係数計算
float calculate_voltage_coefficient(PulseModeReference pmref) {
    float voltage_coefficient = 4.0f / M_PI;
    if (pmref.SvmEnable == false && pmref.ThiEnable == false) {
        voltage_coefficient =
            pmref.mVoltage < 0.786f
                ? 4.0f / M_PI
                : 4.0f / M_PI +
                      pow(1.8477033761888386f *
                              CORRECTIONFACTOR_NORMAL[(byte)min(
                                  213, (pmref.mVoltage - 0.786f) * 1000)] /
                              255,
                          4);
    } else if (pmref.SvmEnable == false && pmref.ThiEnable == true) {
        voltage_coefficient =
            pmref.mVoltage < 0.906f
                ? 4.0f / M_PI
                : 4.0f / M_PI +
                      pow(2.0404186746129294f *
                              CORRECTIONFACTOR_THI[(byte)min(
                                  93, (pmref.mVoltage - 0.906f) * 1000)] /
                              255,
                          4);
    } else if (pmref.SvmEnable == true && pmref.ThiEnable == false) {
        voltage_coefficient =
            pmref.mVoltage < 0.906f
                ? 4.0f / M_PI
                : 4.0f / M_PI +
                      pow(2.040232291671307f *
                              CORRECTIONFACTOR_SVM[(byte)min(
                                  93, (pmref.mVoltage - 0.906f) * 1000)] /
                              255,
                          4);
    }
    return voltage_coefficient;
}

void UpdatePwmMode(PulseModeReference pmref, pwm_config* pm) {
    pmref.mVoltage = max(min(pmref.mVoltage, 1), 0);
    pm->carrier_freq_hz = pmref.fCarrier;
    pm->signal_freq_hz = pmref.fSig;
    pm->modulation_index =
        pmref.mVoltage * calculate_voltage_coefficient(pmref) / (2 * 127);
    pm->sig_mode = min(pmref.SvmEnable * 2 + pmref.ThiEnable * 1, 2);

    uint16_t top_val = 0;
    if (pm->carrier_freq_hz > 0) {
        top_val = (uint16_t)(TIMER_CLOCK / (2.0f * pm->carrier_freq_hz));
    }
    pm->top = top_val;

    uint32_t increment = 0;
    if (pm->carrier_freq_hz > 0) {
        increment =
            (uint32_t)((pm->signal_freq_hz / (2.0f * pm->carrier_freq_hz)) *
                       256 * SHIFT);
    }
    pm->increment = increment;
}

#endif  // PWM_CONTROLLER_H