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

//最大キャリア周波数
#define MAX_FC 1800

//最大信号周波数
#define MAX_FSIG 100

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
    // 量子化誤差低減のため3乗している。3には特別な意味はない
    voltage_coefficient = pmref.mVoltage < 0.786f ? 4.0f / M_PI : 4.0f / M_PI + pow(2.267303569733523f * CORRECTIONFACTOR_NORMAL[(byte)min(213, (pmref.mVoltage - 0.786f) * 1000)] / 255, 3);
  } else if (pmref.SvmEnable == false && pmref.ThiEnable == true) {
    voltage_coefficient = pmref.mVoltage < 0.906f ? 4.0f / M_PI : 4.0f / M_PI + pow(1.9451517598583832f * CORRECTIONFACTOR_THI[(byte)min(92, (pmref.mVoltage - 0.907f) * 1000)] / 255, 3);
  } else if (pmref.SvmEnable == true && pmref.ThiEnable == false) {
    voltage_coefficient = pmref.mVoltage < 0.906f ? 4.0f / M_PI : 4.0f / M_PI + pow(1.9439334827663977f * CORRECTIONFACTOR_SVM[(byte)min(92, (pmref.mVoltage - 0.907f) * 1000)] / 255, 3);
  }
  return voltage_coefficient;
}

//参照/指令パラメータ(pmref)->実行構成(pm)への変換
void UpdatePwmMode(PulseModeReference pmref, pwm_config* pm) {
  pmref.mVoltage = max(min(pmref.mVoltage, 1), 0);
  pm->carrier_freq_hz = min(pmref.fCarrier, MAX_FC);
  pmref.fSig = min(pmref.fSig, MAX_FSIG);
  pm->signal_freq_hz = pmref.fSig;
  pm->modulation_index = pmref.mVoltage * calculate_voltage_coefficient(pmref) / (2 * 127);
  pm->sig_mode = min(pmref.SvmEnable * 2 + pmref.ThiEnable * 1, 2);

  uint16_t top_val = 0;
  if (pm->carrier_freq_hz > 0) {
    top_val = (uint16_t)(TIMER_CLOCK / (2.0f * pm->carrier_freq_hz));
  }
  pm->top = top_val;

  uint32_t increment = 0;
  if (pm->carrier_freq_hz > 0) {
    increment =
      (uint32_t)((pm->signal_freq_hz / (2.0f * pm->carrier_freq_hz)) * SIN_LENGTH * SHIFT);
    //(uint32_t)((pm->signal_freq_hz / (1000)) * SIN_LENGTH * SHIFT);
  }
  pm->increment = increment;
}

#endif  // PWM_CONTROLLER_H
