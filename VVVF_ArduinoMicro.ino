#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <avr/pgmspace.h>
#include "ModController.h"



pwm_config pm;
PulseModeReference pmref;

// 位相アキュムレータ (16ビットで高精度を確保)
volatile uint32_t phase_accumulator = 0;

// --- プロトタイプ宣言 ---
void setup_timer1_3phase();
void update_carrier_frequency();
void update_signal_frequency();
void update_duties_and_set_ocr();

int main() {
  cli();
  pmref.mVoltage = 0.3f;
  pmref.fCarrier = 400.0f;
  pmref.fSig = 15.0f;
  pmref.SvmEnable = 0;
  pmref.ThiEnable = 1;
  setup_timer1_3phase();
  sei();
  while (1) {
    pmref.fSig = pmref.fSig + 1;
    pmref.mVoltage = pmref.fSig / 60;
  }
  return 0;
}

// タイマ1の初期化
void setup_timer1_3phase() {
  
  USBCON &= ~(1 << USBE);  // USBマクロを無効化
  PLLCSR = 0;              // PLLを無効化
  UDIEN = 0;               // USB割り込みをすべて無効化

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

  ICR1 = pm_hold.top;

  phase_accumulator += pm_hold.increment;
  phase_accumulator &= 256 * SHIFT - 1;  //剰余

  uint8_t phase_index = (uint8_t)(phase_accumulator / SHIFT);

  float duty_u = ((float)(pgm_read_byte_near(&SIN_U[pm_hold.sig_mode][phase_index]) - 127) * pm_hold.modulation_index / 127.0f + 0.5f);
  float duty_v = ((float)(pgm_read_byte_near(&SIN_V[pm_hold.sig_mode][phase_index]) - 127) * pm_hold.modulation_index / 127.0f + 0.5f);
  float duty_w = ((float)(pgm_read_byte_near(&SIN_W[pm_hold.sig_mode][phase_index]) - 127) * pm_hold.modulation_index / 127.0f + 0.5f);
  duty_u = max(min(duty_u, 1), 0);
  duty_v = max(min(duty_v, 1), 0);
  duty_w = max(min(duty_w, 1), 0);

  uint16_t ocr_a = (uint16_t)(duty_u * pm_hold.top);
  uint16_t ocr_b = (uint16_t)(duty_v * pm_hold.top);
  uint16_t ocr_c = (uint16_t)(duty_w * pm_hold.top);

  OCR1A = ocr_a;
  OCR1B = ocr_b;
  OCR1C = ocr_c;
}

// カウンタがBOTTOM(谷)に達したとき
ISR(TIMER1_OVF_vect) {
  update_duties_and_set_ocr();
  TCCR1A = 0b10101010;  // 次の更新はTOP(山)
}

// カウンタがTOP(山)に達したとき
ISR(TIMER1_CAPT_vect) {
  update_duties_and_set_ocr();
  TCCR1A = 0b10101000;  // 次の更新はBOTTOM(谷)
}