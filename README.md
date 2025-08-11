# Arduino MicroでVVVF（非同期のみ）

## 概要
* Arduino Micro (ATmega32U4) で三相PWMを生成し、VVVF用の三相出力(U/V/W)を出力する。
* Timer1: 位相基準PWMor位相/周波数基準PWM (TOP=ICR1) により OC1A/OC1B/OC1C で3相を出力。
* Timer3: 一定周期 (samplingRate [Hz]) でパラメータ更新要求フラグを立て、loopで反映。
* 波形生成: DDS方式により基本波の位相を進め、変調率を掛けてDutyに変換。
* pwm_controller.h: 参照/指令パラメータ (pmref) から、TOP値/increment/modulation_indexなど実行用構成 (pm) を決定。

## ピン対応とか
| ピン | ポート | 機能 |
----|----|----
|D9|OC1A|U相|
|D10|OC1B|V相|
|D11|OC1C|W相|

![alt](https://github.com/IR2110/VVVF_ArduinoMicro/blob/main/%E3%83%94%E3%83%B3%E9%85%8D%E7%BD%AE.png)

## 注意
- Timer1はなぜかUSBに干渉するクソ仕様なので書き込み時にリセットボタンを2回押さないといけない。
- 信号波周波数100Hzまで加速して、そのまま100Hzを出力し続ける。
- デッドタイムの生成はハードウェア制約からArduino側で行わない。
