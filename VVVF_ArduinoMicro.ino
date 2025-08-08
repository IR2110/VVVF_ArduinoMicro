#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "pwm_controller.h"

pwm_config pm;
PulseModeReference pmref;

// 位相アキュムレータ（DDSよろしくのアレ）
volatile uint32_t phase_accumulator = 0;

const float samplingRate = 100.0f;

void setup_timer1_3phase();
void update_carrier_frequency();
void update_signal_frequency();
void update_duties_and_set_ocr();
void setup_timer3_param_update();

void setup() {
    cli();
    pmref.mVoltage = 0.0f;
    pmref.fCarrier = 1050.0f;
    pmref.fSig = 0.0f;
    pmref.SvmEnable = 0;
    pmref.ThiEnable = 1;
    setup_timer1_3phase();
    setup_timer3_param_update();
    sei();
}

void loop() {}

void update() {  //  samplingRate [Hz]ごとに呼ばれる
    pmref.fSig += 4 / samplingRate;
    pmref.mVoltage = pmref.fSig / 60.0f + 0.02f;
    UpdatePwmMode(pmref, &pm);
}

// タイマ1の初期化
void setup_timer1_3phase() {
    UpdatePwmMode(pmref, &pm);
    update_duties_and_set_ocr();
    USBCON &= ~(1 << USBE);  // USBマクロを無効化
    PLLCSR = 0;              // PLLを無効化
    UDIEN = 0;               // USB割り込みをすべて無効化

    DDRB |= (1 << DDB5) | (1 << DDB6) | (1 << DDB7);  // OC1A/B/Cを出力に設定

    // TCCR1A/Bレジスタ設定
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << COM1C1);  // 次の更新はBOTTOM（谷）
    TCCR1B = (1 << WGM13) | (1 << CS11);                     // 位相基準PWM（TOP=ICR1）、プリスケーラ8

    // 割り込み許可
    TIMSK1 = (1 << ICIE1) | (1 << TOIE1);
}

// パラメータ更新用のタイマー3の設定
void setup_timer3_param_update() {
    // CTCモード、プリスケーラ64
    TCCR3A = 0;
    TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);

    // (16000000 / 64) / 100Hz = 2500
    OCR3A = (uint16_t)((F_CPU / 64) / samplingRate);

    // タイマー3の比較A割り込みを有効化
    TIMSK3 = (1 << OCIE3A);
}

// 割り込み処理共通
void update_duties_and_set_ocr() {
    pwm_config pm_hold = pm;  // バッファをとっておいて、これを実行中にupdateが走ってpmが変更されても影響しないように

    ICR1 = pm_hold.top;

    phase_accumulator += pm_hold.increment;
    phase_accumulator &= SIN_LENGTH * SHIFT - 1;  // 剰余

    uint8_t phase_index = (uint8_t)(phase_accumulator / SHIFT);

    //0~top
    uint16_t duty_u = (uint16_t)(((float)(pgm_read_byte_near(&SIN_U[pm_hold.sig_mode][phase_index]) - LUT_ZERO_LEVEL) * pm_hold.modulation_index + 0.5f) * pm_hold.top);
    uint16_t duty_v = (uint16_t)(((float)(pgm_read_byte_near(&SIN_V[pm_hold.sig_mode][phase_index]) - LUT_ZERO_LEVEL) * pm_hold.modulation_index + 0.5f) * pm_hold.top);
    uint16_t duty_w = (uint16_t)(((float)(pgm_read_byte_near(&SIN_W[pm_hold.sig_mode][phase_index]) - LUT_ZERO_LEVEL) * pm_hold.modulation_index + 0.5f) * pm_hold.top);

    // 過変調してもタイマがぶっこわれないように制限
    duty_u = max(min(duty_u, pm_hold.top), 0);
    duty_v = max(min(duty_v, pm_hold.top), 0);
    duty_w = max(min(duty_w, pm_hold.top), 0);

    // 出力コンペアレジスタに設定
    OCR1A = duty_u;
    OCR1B = duty_v;
    OCR1C = duty_w;
}

// カウンタがBOTTOM（谷）に達したとき
ISR(TIMER1_OVF_vect) {
    update_duties_and_set_ocr();
    TCCR1A = 0b10101010;  // 次の更新はTOP（山）
}

// カウンタがTOP（山）に達したとき
ISR(TIMER1_CAPT_vect) {
    update_duties_and_set_ocr();
    TCCR1A = 0b10101000;  // 次の更新はBOTTOM（谷）
}

ISR(TIMER3_COMPA_vect) {  // updateをsamplingRate [Hz]で呼ぶ
    update();
}
