#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "pwm_controller.h"

/*
  概要:
    - Arduino Micro (ATmega32U4)
  で三相中心対称PWMを生成し、VVVF用の三相出力(U/V/W)を出力する。
    - Timer1: 位相基準PWM (Phase and Frequency Correct PWM, TOP=ICR1) により
  OC1A/OC1B/OC1C で3相を出力。
    - Timer3: 一定周期 (samplingRate [Hz])
  でパラメータ更新要求フラグを立て、メインループで安全に反映。
    - 波形生成: DDS方式 (phase_accumulator と SIN_* LUT)
  により基本波の位相を進め、変調率を掛けてデューティに変換。
    - pwm_controller.h: 参照/指令パラメータ (pmref)
  から、TOP/インクリメント/変調率など実行用構成 (pm) を決定。

  ポート/ピン対応 (32U4):
    - D9  -> OC1A (U相)
    - D10 -> OC1B (V相)
    - D11 -> OC1C (W相)

  注意:
    - 信号波周波数100Hzまで加速して、そのまま100Hzを出力し続ける。
    - デッドタイムの生成はハードウェア制約からArduino側で行わない。
*/

pwm_config pm;
PulseModeReference pmref;

// 位相アキュムレータ（DDSよろしくのアレ）
volatile uint32_t phase_accumulator = 0;

// サンプリングレート [Hz]
const float samplingRate = 100.0f;

// update実行したいなフラグ
volatile bool param_update_pending = false;

void setup_timer1_3phase();
void update_carrier_frequency();
void update_signal_frequency();
void update_duties_and_set_ocr();
void setup_timer3_param_update();

void setup() {
	cli();
	pmref.mVoltage = 0.0f;
	pmref.fCarrier = 1000.0f;
	pmref.fSig = 0.0f;
    pmref.SvmEnable = 0;
	pmref.ThiEnable = 0;
	UpdatePwmMode(pmref, &pm);
	setup_timer1_3phase();
	setup_timer3_param_update();
	sei();
}

void loop() {
	// フラグが立っていたら1回分処理
	if (param_update_pending) {
		uint8_t sreg = SREG;  //ステータス・レジスタを保存
		cli();
		param_update_pending = false;
		SREG = sreg;  //ステータス・レジスタを復元

		update();

		pwm_config new_pm;
		UpdatePwmMode(pmref, &new_pm);

		sreg = SREG;  //ステータス・レジスタを保存
		cli();
		pm = new_pm;  //新しいPWM設定をまとめて適用
		SREG = sreg;  //ステータス・レジスタを復元
	}
}

void update() {  //  samplingRate [Hz]ごとに呼ばれる
	pmref.fSig += 4 / samplingRate;
	pmref.mVoltage = pmref.fSig / 80.0f + 0.02f;

	float fsw = 800.0f;
	if (pm.modulation_index * (2 * LUT_ZERO_LEVEL) > 1) {
		//過変調になっても平均SW回数を一定にする（謎こだわり）
		pmref.fCarrier = fsw * (M_PI / 2) / asin(max(min(1 / (pm.modulation_index * (2 * 127)), 1), -1));
	} else {
		pmref.fCarrier = fsw;
		if (pmref.mVoltage < 0.2) pmref.fCarrier = 150 + (pmref.mVoltage - 0.05) * (fsw - 150) / 0.15;  //低変調率のときは低Fcにしたほうが効率良かったりする
		if (pmref.mVoltage < 0.05) pmref.fCarrier = 150;
	}
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

	OCR3A = (uint16_t)((F_CPU / 64) / samplingRate);

	// タイマー3の比較A割り込みを有効化
	TIMSK3 = (1 << OCIE3A);
}

// 割り込み処理共通
void update_duties_and_set_ocr() {
	pwm_config pm_hold = pm;  // バッファをとっておいて、これを実行中にupdateが走ってpmが変更されても影響しないように

	phase_accumulator += pm_hold.increment;       // θ積分
	phase_accumulator &= SIN_LENGTH * SHIFT - 1;  // 剰余

	uint8_t phase_index = (uint8_t)(phase_accumulator / SHIFT);

	//デューティー計算
	float duty_u = (((float)(pgm_read_byte_near(&SIN_U[pm_hold.sig_mode][phase_index]) - LUT_ZERO_LEVEL) * pm_hold.modulation_index + 0.5f));
	float duty_v = (((float)(pgm_read_byte_near(&SIN_V[pm_hold.sig_mode][phase_index]) - LUT_ZERO_LEVEL) * pm_hold.modulation_index + 0.5f));
	float duty_w = (((float)(pgm_read_byte_near(&SIN_W[pm_hold.sig_mode][phase_index]) - LUT_ZERO_LEVEL) * pm_hold.modulation_index + 0.5f));

	// 過変調してもタイマがぶっこわれないように制限
	duty_u = max(min(duty_u, 1), 0);
	duty_v = max(min(duty_v, 1), 0);
	duty_w = max(min(duty_w, 1), 0);

	// 出力コンペアレジスタに設定
	OCR1A = (uint16_t)(duty_u * pm_hold.top);
	OCR1B = (uint16_t)(duty_v * pm_hold.top);
	OCR1C = (uint16_t)(duty_w * pm_hold.top);
	// TOP値を設定
	ICR1 = pm_hold.top;
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

ISR(TIMER3_COMPA_vect) {  // update実行したいなフラグをsamplingRate
	                      // [Hz]ごとに立てる
	param_update_pending = true;
}
