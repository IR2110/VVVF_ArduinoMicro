#include <avr/io.h>
#include <avr/interrupt.h>

float f = 10;  // 割り込み周波数
void setup() {
  // LEDピンを出力モード
  DDRC |= (1 << DDC7);

  // 高速PWMモード TOP=ICR1, 分周比=64
  // WGM11=1, WGM10=0
  TCCR1A = 0b00000010;
  // WGM13=1, WGM12=0, CS12=0, CS11=1, CS10=1
  TCCR1B = 0b00011011;

  // TOP値設定
  ICR1 = (uint16_t)(16000000 / (64 * f)) - 1;

  // 割り込みを有効にする（TOIE1=1）
  TIMSK1 = 0b00000001;
}
void loop() {
  // 何もしなくてもLEDが点滅するハズ
}
ISR(TIMER1_OVF_vect) {  // 割り込みサービスルーチン
  PINC |= (1 << PC7);   // LEDをトグル
}