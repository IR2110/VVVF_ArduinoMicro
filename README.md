# Arduino MicroでVVVF
「インバータ技術2025年12月号 中学生でも作れる！VVVFインバータの創り方」 付属サンプルプログラム集

# 各プログラムの概要
## Pwm_1kHz
ファイルパス: `Pwm_1kHz/Pwm_1kHz.ino`

Arduino Microで1kHz、デューティ比50%のPWM信号を生成。

## Timer_Interrupt
ファイルパス: `Timer_Interrupt/Timer_Interrupt.ino`

タイマ割り込みを使って0.1sごとに13番ピン（緑色LED）をチカチカする。いわゆるLチカ。

## VVVF_test1
ファイルパス: `VVVF_test1/VVVF_test1.ino`

3相PWMのテスト用プログラム。位相アキュムレータ知らない感じで実装してるので周波数分解能が7.8Hz。波形としてはそれっぽいけど多分ぶっ壊れるので使わないでね。

## VVVF_test2
ファイルパス: `VVVF_test2/VVVF_test2.ino`

正式な3相PWM生成用プログラム。位相アキュムレータとかちゃんとしたバージョン。

## VVVF_Completed
ファイルパス: `VVVF_Completed/VVVF_Completed.ino`

THI・SVMや電圧補償係数の実装もしてあるArduino Microで非同期PWMを出すためにいろいろ頑張ったやつ。発展型。

## SampleInverter
ファイルパス: `SampleInverter/SampleInverter.kicad_pro`

本書で作ったインバータのサンプルプロジェクト
