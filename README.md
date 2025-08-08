# Arduino MicroでVVVF（非同期のみ）

## 概要
* Arduino Micro (ATmega32U4) で三相PWMを生成し、VVVF用の三相出力(U/V/W)を出力する。
* Timer1: 位相基準PWM (Phase and Frequency Correct PWM, TOP=ICR1) により OC1A/OC1B/OC1C で3相を出力。
* Timer3: 一定周期 (samplingRate [Hz]) でパラメータ更新要求フラグを立て、メインループで安全に反映。
* 波形生成: DDS方式 (phase_accumulator と SIN_* LUT) により基本波の位相を進め、変調率を掛けてデューティに変換。
* pwm_controller.h: 参照/指令パラメータ (pmref) から、TOP/インクリメント/変調率など実行用構成 (pm) を決定。

## ピン対応とか
| ピン | ポート | 機能 |
----|----|----
|D9|OC1A|U相|
|D10|OC1B|V相|
|D11|OC1C|W相|

![alt](https://github.com/IR2110/VVVF_ArduinoMicro/blob/main/%E3%83%92%E3%82%9A%E3%83%B3%E9%85%8D%E7%BD%AE.png)

## 注意:
- Timer1はなぜかUSBに干渉するクソ仕様なので書き込み時にリセットボタンを2回押さないといけない。
- 信号波周波数100Hzまで加速して、そのまま100Hzを出力し続ける。
- デッドタイムの生成はハードウェア制約からArduino側で行わない。
