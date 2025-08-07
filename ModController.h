
#include <math.h>
#include "LUT.h"

// 本番のPWM生成に使うためのちゃんと整形されたPWM設定。
typedef struct pwm_config {
  volatile float carrier_freq_hz;
  volatile float signal_freq_hz;
  volatile float modulation_index;  // 信号波振幅
  volatile uint8_t sig_mode;
} pwm_config;

//PWMパターン設定から送られてくるナマのPWM設定
typedef struct PulseModeReference {
  volatile float fCarrier;
  volatile float fSig;
  volatile float mVoltage;
  volatile bool ThiEnable;
  volatile bool SvmEnable;
} PulseModeReference;

float calculate_voltage_coefficient(PulseModeReference pmref) {
  float voltage_coefficient = 4.0f / M_PI;

  if (pmref.SvmEnable == false && pmref.ThiEnable == false) {
    voltage_coefficient = pmref.mVoltage < 0.786f ? 4.0f / M_PI : 4.0f / M_PI + pow(1.8477033761888386f * CORRECTIONFACTOR_NORMAL[(byte)min(213, (pmref.mVoltage - 0.786f) * 1000)] / 255, 4);
  } else if (pmref.SvmEnable == false && pmref.ThiEnable == true) {
    voltage_coefficient = pmref.mVoltage < 0.906f ? 4.0f / M_PI : 4.0f / M_PI + pow(2.0404186746129294f * CORRECTIONFACTOR_THI[(byte)min(93, (pmref.mVoltage - 0.906f) * 1000)] / 255, 4);
  } else if (pmref.SvmEnable == true && pmref.ThiEnable == false) {
    voltage_coefficient = pmref.mVoltage < 0.906f ? 4.0f / M_PI : 4.0f / M_PI + pow(2.040232291671307f * CORRECTIONFACTOR_SVM[(byte)min(93, (pmref.mVoltage - 0.906f) * 1000)] / 255, 4);
  }

  return voltage_coefficient;
}

void UpdatePwmMode(PulseModeReference pmref, pwm_config* pm) {
  pm->carrier_freq_hz = pmref.fCarrier;
  pm->signal_freq_hz = pmref.fSig;
  pm->modulation_index = pmref.mVoltage * calculate_voltage_coefficient(pmref);
  pm->sig_mode = min(pmref.SvmEnable * 2 + pmref.ThiEnable * 1,2);
}

