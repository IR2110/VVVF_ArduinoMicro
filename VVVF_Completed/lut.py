import pyperclip
import numpy as np

def to_c_array_and_copy(data, var_name="lut", data_type="int", items_per_line=16):
    """
    Pythonのリスト（多次元配列対応）をC言語の配列定義文字列に変換し、中身をクリップボードにコピーするよ。

    Args:
        data (list): 配列に変換するデータのリスト
        var_name (str): C言語での変数名。
        data_type (str): C言語でのデータ型。
        items_per_line (int): 1行に表示する要素の数（最内周の配列に適用）。
    """

    def _get_dims(arr):
        """配列の次元を取得する"""
        if not isinstance(arr, list):
            return []
        dims = []
        current = arr
        while isinstance(current, list):
            dims.append(len(current))
            if len(current) == 0:
                break
            current = current[0]
        return dims

    def _format_recursive(arr, level=0):
        """配列を再帰的にフォーマットする"""
        if not isinstance(arr, list):
            return str(arr)

        # 配列の要素を再帰的に処理
        elements = [_format_recursive(item, level + 1) for item in arr]

        # 最内周の配列の場合、items_per_line に従って改行を入れる
        is_innermost = not any(isinstance(item, list) for item in arr)
        if is_innermost:
            formatted_elements = []
            for i in range(0, len(elements), items_per_line):
                chunk = elements[i:i + items_per_line]
                # インデントを追加して結合
                formatted_elements.append("    " * (level+1) + ", ".join(chunk))
            content = ",".join(formatted_elements)
        else:
            # 外側の配列は、各要素を改行で区切る
            content = ",\n".join(elements)

        indent = "    " * level
        return f"{{{content}{indent}}}"


    # 配列の次元を取得してC言語の変数宣言を作成
    dims = _get_dims(data)
    dim_str = "".join([f"[{d}]" for d in dims])

    # 配列の中身の文字列を組み立てる
    string_to_copy = _format_recursive(data)

    # 表示用の完全なC言語配列定義文字列
    full_c_array_string = f"{data_type} {var_name}{dim_str} = {string_to_copy};"

    # クリップボードにコピー
    try:
        pyperclip.copy(string_to_copy)
        print(f"C言語形式の配列 '{var_name}' の中身をクリップボードにコピーしました。")
        print("--- コピーされた内容 ---")
        print(string_to_copy)
        print("-----------------------")
        # print("\n--- 完全なC言語の定義（参考） ---")
        # print(full_c_array_string)
        # print("---------------------------------")
    except pyperclip.PyperclipException:
        print("エラー: pyperclipライブラリが見つかりません。")
        print("次のコマンドでインストールしてください: pip install pyperclip")
        print("\n--- 生成されたコード（中身） ---")
        print(string_to_copy)
        print("--------------------------")
        print("\n--- 完全なC言語の定義（参考） ---")
        print(full_c_array_string)
        print("---------------------------------")


# --- ここから設定 ---


x = np.arange(0, 256)
# サイン波を計算 (-1.0 to 1.0) -> スケール (0 to 255) -> 整数に変換
def wave(t, mode):
    u = 0
    v = 0
    w = 0
    u = np.sin(t)
    v = np.sin(t - 2*np.pi/3)
    w = np.sin(t + 2*np.pi/3)
    if mode == 0:
        z = 0 #正弦波PWM
    elif mode == 1:
        z = np.sin(3*t)/6 #1/6重畳THI
    elif mode == 2:
        z = (np.max([u,v,w]) + np.min([u,v,w]))*-0.5 #SVM
    # z = np.sin(3*t)/6 #1/6重畳THI
    # z = np.sin(3*t)/4 #1/4重畳THI
    # z = (np.max([u,v,w]) + np.min([u,v,w]))*-0.5 #SVM
    return u+z

spwm = []
for i in range(256):
    # 角度を計算 (0 to 2π)
    t = i / 256.0 * 2 * np.pi
    # 波形を計算し、スケールを調整して整数に変換
    value = int((wave(t, 0) * 127) + 127)
    spwm.append(value)

thi = []
for i in range(256):
    # 角度を計算 (0 to 2π)
    t = i / 256.0 * 2 * np.pi
    # 波形を計算し、スケールを調整して整数に変換
    value = int((wave(t, 1) * 127) + 127)
    thi.append(value)

svm = []
for i in range(256):
    # 角度を計算 (0 to 2π)
    t = i / 256.0 * 2 * np.pi
    # 波形を計算し、スケールを調整して整数に変換
    value = int((wave(t, 2) * 127) + 127)
    svm.append(value)


data = [spwm, thi, svm]

import matplotlib.pyplot as plt

# グラフ描画
plt.figure(figsize=(12, 7))
plt.title("PWM Waveforms")
plt.xlabel("Phase [index]")
plt.ylabel("Value")

labels = ["SPWM", "THI", "SVM"]
x = np.arange(len(data[0]))

for i, waveform in enumerate(data):
    plt.step(x, waveform, where='post', label=labels[i], linewidth=1.2)
    # plt.plot(x, waveform, 'o', markersize=2, alpha=0.6)  # サンプル点を表示して離散性を強調

plt.legend()
plt.grid(True, linestyle='--', alpha=0.5)
plt.xlim(0, len(x)-1)
plt.tight_layout()
plt.ylim(0, 255)
plt.show()

# C言語での配列の変数名
c_variable_name = "sine_wave_lut"

# C言語での配列のデータ型 (例: "int", "unsigned char", "float")
c_data_type = "const int16_t"

# 1行あたりに表示する要素の数
c_items_per_line = 16000000000

# --- ここまで設定 ---


# 関数を実行して、C言語の配列を生成しクリップボードにコピー
if __name__ == "__main__":
    to_c_array_and_copy(
        data,
        c_variable_name,
        c_data_type,
        c_items_per_line
    )
