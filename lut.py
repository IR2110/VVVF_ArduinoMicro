import pyperclip
import numpy as np

def to_c_array_and_copy(data, var_name="lut", data_type="int", items_per_line=16):
    """
    PythonのリストをC言語の配列定義文字列に変換し、中身をクリップボードにコピーします。

    Args:
        data (list): 配列に変換するデータのリスト。
        var_name (str): C言語での変数名。
        data_type (str): C言語でのデータ型。
        items_per_line (int): 1行に表示する要素の数。
    """
    # 配列の要素を文字列に変換
    elements = [str(item) for item in data]

    # 見やすくするために、指定した数ごとに改行を入れる
    formatted_elements = []
    for i in range(0, len(elements), items_per_line):
        chunk = elements[i:i + items_per_line]
        # インデントを追加して結合
        formatted_elements.append("    " + ", ".join(chunk))

    # 配列の中身の文字列を組み立てる
    content_inside_braces = ",\n".join(formatted_elements)

    # クリップボードにコピーする文字列 ({ ... } の部分)
    string_to_copy = f"{{{content_inside_braces}}}"

    # 表示用の完全なC言語配列定義文字列
    full_c_array_string = f"{data_type} {var_name}[{len(data)}] = {string_to_copy};"

    # クリップボードにコピー
    try:
        pyperclip.copy(string_to_copy)
        print(f"C言語形式の配列 '{var_name}' の中身をクリップボードにコピーしました。")
        print("--- コピーされた内容 ---")
        print(string_to_copy)
        print("-----------------------")
        print("\n--- 完全なC言語の定義（参考） ---")
        print(full_c_array_string)
        print("---------------------------------")
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

# C言語の配列に変換したいPythonのリストをここに記述します
# 例: my_data = [10, 20, 30, 40, 50]
# 例: my_data = list(range(100)) # 0から99までの数値を生成
# 0から255までの数値でサイン波を生成し、0-255の範囲にスケーリングします
x = np.arange(0, 256)
# サイン波を計算 (-1.0 to 1.0) -> スケール (0 to 255) -> 整数に変換
def wave(t):
    u = np.sin(t)
    v = np.sin(t - 2*np.pi/3)
    w = np.sin(t + 2*np.pi/3)
    z = 0 #正弦波PWM
    z = np.sin(3*t)/6 #1/6重畳THI
    # z = np.sin(3*t)/4 #1/4重畳THI
    z = (np.max([u,v,w]) + np.min([u,v,w]))*-0.5 #SVM
    return w + z


my_data = ((wave(np.arange(0, 256) / 256 * 2 * np.pi)) * 128).astype(int)

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
        my_data,
        c_variable_name,
        c_data_type,
        c_items_per_line
    )