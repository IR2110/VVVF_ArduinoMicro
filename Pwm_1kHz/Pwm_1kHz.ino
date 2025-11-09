float f = 1000;  // PWM周波数
float d = 0.5;   // デューティ比
void setup() {
  // OC1Aピンを出力モード
  DDRB |= (1 << DDB5);  // PB5を出力モード

  // COM1A1=1, COM1A0=0, WGM11=0, WGM10=0
  TCCR1A = 0b10000000;

  // WGM13=1, WGM12=0, CS12=0, CS11=0, CS10=1
  TCCR1B = 0b00010001;

  // TOP値設定
  ICR1 = (uint16_t)(16000000 / (2 * 1 * f));

  // デューティ比設定
  OCR1A = (uint16_t)(d * ICR1);
}
void loop() {
  // 何もしなくてもPWM信号がD9(OC1A)から出力されるハズ
}