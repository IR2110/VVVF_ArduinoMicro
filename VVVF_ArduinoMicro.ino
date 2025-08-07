#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <avr/pgmspace.h>
#include "ModController.h"

// --- 基本パラメータ設定 ---
#define F_CPU 16000000UL
#define PRESCALER_VALUE 8
#define TIMER_CLOCK (F_CPU / PRESCALER_VALUE)

pwm_config pm;
PulseModeReference pmref;

// 位相アキュムレータ (16ビットで高精度を確保)
volatile uint16_t phase_accumulator = 0;

// --- プロトタイプ宣言 ---
void setup_timer1_3phase();
void update_carrier_frequency();
void update_signal_frequency();
void update_duties_and_set_ocr();

int main() {

  pmref.mVoltage = 0.7f;
  pmref.fCarrier = 2000.0f;
  pmref.fSig = 50.0f;
  pmref.SvmEnable = 1;
  setup_timer1_3phase();
  sei();
  while (1) {
  }
  return 0;
}

// タイマ1の初期化
void setup_timer1_3phase() {
  init();
      USBCON &= ~(1 << USBE); // USBマクロを無効化
    PLLCSR = 0;             // PLLを無効化
    UDIEN = 0;              // USB割り込みをすべて無効化

  DDRB |= (1 << DDB5) | (1 << DDB6) | (1 << DDB7);  // OC1A/B/Cを出力に設定


  UpdatePwmMode(pmref, &pm);
  update_duties_and_set_ocr();

  // TCCR1A/Bレジスタ設定
  TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << COM1C1);
  TCCR1A = 0b10101010;                  // 次の更新はBOTTOM(谷)
  TCCR1B = (1 << WGM13) | (1 << CS11);  // 位相基準PWM(TOP=ICR1), プリスケーラ8

  // 割り込み許可
  TIMSK1 = (1 << ICIE1) | (1 << TOIE1);
}


// 割り込み処理共通
void update_duties_and_set_ocr() {
  pwm_config pm_hold = pm;

  uint16_t top_val = 0;
  if (pm_hold.carrier_freq_hz > 0) {
    top_val = (uint16_t)(TIMER_CLOCK / (2.0f * pm_hold.carrier_freq_hz));
  }

  ICR1 = top_val - 1;

  uint16_t increment = 0;
  if (pm_hold.carrier_freq_hz > 0) {
    increment = (uint16_t)((pm_hold.signal_freq_hz / (2.0f * pm_hold.carrier_freq_hz)) * 65536);
  }
  phase_accumulator += increment;

  // 32ビットアキュムレータの上位8ビットをサインテーブルのインデックスとして使用
  uint8_t phase_index = (uint8_t)(phase_accumulator >> 8);

  float duty_u = ((float)pgm_read_byte_near(&SIN_U[2][phase_index]) * pm_hold.modulation_index / 256.0f + 0.5f);
  float duty_v = ((float)pgm_read_byte_near(&SIN_V[2][phase_index]) * pm_hold.modulation_index / 256.0f + 0.5f);
  float duty_w = ((float)pgm_read_byte_near(&SIN_W[2][phase_index]) * pm_hold.modulation_index / 256.0f + 0.5f);
  duty_u = max(min(duty_u, 1), 0);
  duty_v = max(min(duty_v, 1), 0);
  duty_w = max(min(duty_w, 1), 0);

  uint16_t ocr_a = (uint16_t)(duty_u * (float)top_val);
  uint16_t ocr_b = (uint16_t)(duty_v * (float)top_val);
  uint16_t ocr_c = (uint16_t)(duty_w * (float)top_val);

  OCR1A = ocr_a - 1;
  OCR1B = ocr_b - 1;
  OCR1C = ocr_c - 1;
}

// カウンタがBOTTOM(谷)に達したとき
ISR(TIMER1_OVF_vect) {
  update_duties_and_set_ocr();
  //TCCR1A = 0b10101010;  // 次の更新はTOP(山)
}

// カウンタがTOP(山)に達したとき
ISR(TIMER1_CAPT_vect) {
  update_duties_and_set_ocr();
  TCCR1A = 0b10101000;  // 次の更新はBOTTOM(谷)
}