#過変調リニアリティ補正用テーブル生成プログラム

import numpy as np
from scipy.optimize import fsolve
from scipy.integrate import quad
from scipy.optimize import minimize_scalar
import csv

#信号波
def wave(t):
    u = np.sin(t)
    v = np.sin(t - 2*np.pi/3)
    w = np.sin(t + 2*np.pi/3)
    z = 0 #正弦波PWM
    # z = np.sin(3*t)/6 #1/6重畳THI
    # z = np.sin(3*t)/4 #1/4重畳THI
    z = (np.max([u,v,w]) + np.min([u,v,w]))*-0.5 #SVM
    return u + z

#フーリエ計算用
def forier(t, a):
    return np.sin(t)*min(1,a*wave(t))


def equation(a, x, max_wave_val):
    # a * max_wave_val が 1 以下の場合、過変調領域ではない
    if a * max_wave_val <= 1:
        return x - a * np.pi/4

    # 積分計算
    s, _ = quad(forier, 0, np.pi/2, args=(a,))

    # fsolveは f(x) = 0 となるxを探すため、方程式 x - s = 0 を評価
    return x - s



def solve_equation(m_value, initial_guesses, max_wave_val):
    for initial_guess in initial_guesses:
        try:
            a_solution, info, ier, msg = fsolve(equation, initial_guess, args=(m_value, max_wave_val), full_output=True, maxfev=10000)
            if ier == 1:  # 解が見つかった場合
                return a_solution[0]
        except Exception as e:
            continue  # 次の初期値を試す
    raise RuntimeError(f"Solution not found for m = {m_value}")

# m の範囲
m_values = np.arange(0.001, 1.000, 0.001)
# 初期値のリスト
initial_guesses = np.arange(1.0, 50.0, 0.01)

import matplotlib.pyplot as plt

# 結果を保存するリスト
a_results = []
m_results = []

# 過変調の境界値を求める
res = minimize_scalar(lambda t: -wave(t), bounds=(0, np.pi/2), method='bounded')
max_wave_val = -res.fun
# 各mのときのaを計算
for m_value in m_values:
        try:
            a_value = solve_equation(m_value, initial_guesses, max_wave_val)
            a_results.append(a_value)
            m_results.append(m_value)

        except RuntimeError as e:
            print(f"Error for m = {m_value:.3f}: {e}")


#8bitにうまく格納するための変換用関数
def convert(a,m):
    correction_factor = (a/m)-4/np.pi
    return pow(correction_factor, 1/3)

# CSVファイルを開く
with open('output.csv', 'w', newline='') as f:
    writer = csv.writer(f)

    # a_resultsが1/max_wave_val以下の要素を削除し、対応するm_resultsも同じindexで削除
    threshold = 1.0 / max_wave_val
    filtered = [(m, a) for m, a in zip(m_results, a_results) if a > threshold]
    m_results[:] = [m for m, _ in filtered]
    a_results[:] = [a for _, a in filtered]


    for m_value, a_value in zip(m_results, a_results):
        a_value = convert(a_value, m_value) #電圧補償係数を変換

        a_value *= 255 / convert(max(a_results), max(m_results))
        a_value = int(round(a_value))  # 整数に変換
        writer.writerow([f"{m_value:.3f}", f"{a_value}"])

print(convert(max(a_results), max(m_results)))
# グラフの描画
if a_results:
    plt.figure()

    # 過変調領域のプロット
    plt.plot(a_results, m_results, color='black', linestyle='--', label='Overmodulation region')

    # 線形領域の計算とプロット
    a_boundary = 1/max_wave_val
    m_boundary = a_boundary * np.pi / 4
    a_linear = np.linspace(0, a_boundary, 100)
    m_linear = a_linear * np.pi / 4
    plt.plot(a_linear, m_linear, color='black', linestyle='-', label='Linear region')

    # 過変調の境界線
    plt.axvline(x=a_boundary, color='black', linestyle=':')
    text_str = f'({a_boundary:.4g}, {m_boundary:.4g})'
    plt.text(a_boundary + 0.05, m_boundary, text_str, va='center', color='black')
    # 過変調の境界点
    plt.plot(a_boundary, m_boundary, 'ko')

    plt.xlabel("Modulation Index $A$", math_fontfamily='cm')
    plt.ylabel("Normalized Fundamental Component $M$", math_fontfamily='cm')
    plt.xlim(0, 4)
    plt.ylim(0, 1)
    plt.grid(True)
    plt.legend()
    plt.show()
