import sys
import subprocess
import tkinter as tk
from tkinter import ttk
import sounddevice as sd
import numpy as np
import threading
import time
import winsound
import ctypes

# 适配 Windows 高 DPI
try:
    ctypes.windll.shcore.SetProcessDpiAwareness(1)
    ctypes.windll.user32.SetProcessDPIAware()
except:
    pass

SAMPLE_RATE = 44100
BLOCKSIZE = 1024
VOLUME_THRESHOLD = 0.1
IS_ALARM_PLAYING = False
IS_RUNNING = True

def get_mic_volume():
    """获取麦克风音量（RMS）"""
    try:
        audio_data = sd.rec(int(BLOCKSIZE), samplerate=SAMPLE_RATE, channels=1, dtype='float32')
        sd.wait()
        volume = np.sqrt(np.mean(np.square(audio_data)))
        return float(volume)
    except Exception:
        return 0.0

def volume_monitor_loop():
    """后台音量检测与报警触发"""
    global IS_ALARM_PLAYING
    while IS_RUNNING:
        current_volume = get_mic_volume()
        volume_var.set(f"当前音量: {current_volume:.4f}")

        if current_volume > VOLUME_THRESHOLD:
            if not IS_ALARM_PLAYING:
                IS_ALARM_PLAYING = True
                threading.Thread(target=play_alarm_sound, daemon=True).start()
        else:
            IS_ALARM_PLAYING = False

        time.sleep(0.05)

def play_alarm_sound():
    """播放蜂鸣警报"""
    while IS_ALARM_PLAYING and IS_RUNNING:
        winsound.Beep(1500, 150)

def update_threshold(val):
    """更新报警阈值"""
    global VOLUME_THRESHOLD
    VOLUME_THRESHOLD = round(float(val), 4)
    threshold_var.set(f"报警阈值: {VOLUME_THRESHOLD:.4f}")

def on_closing():
    """安全退出"""
    global IS_RUNNING, IS_ALARM_PLAYING
    IS_RUNNING = False
    IS_ALARM_PLAYING = False
    root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    root.title("音量检测")
    root.geometry("800x400")
    root.resizable(False, False)
    root.configure(bg="#F5F7FA")

    volume_var = tk.StringVar(value="当前音量: 0.0000")
    threshold_var = tk.StringVar(value=f"报警阈值: {VOLUME_THRESHOLD:.4f}")

    title_label = ttk.Label(root, text="音量检测", font=("微软雅黑", 16, "bold"), foreground="#165DFF")
    title_label.pack(pady=12)

    volume_label = tk.Label(root, textvariable=volume_var, font=("微软雅黑", 13),
                            bg="#F5F7FA", fg="#2C3E50", anchor="center")
    volume_label.pack(pady=4)

    threshold_label = tk.Label(root, textvariable=threshold_var, font=("微软雅黑", 13),
                               bg="#F5F7FA", fg="#E67E22", anchor="center")
    threshold_label.pack(pady=4)

    threshold_slider = tk.Scale(root, from_=0.0, to=0.2, orient="horizontal", length=450,
                                command=update_threshold, resolution=0.001, font=("微软雅黑", 10),
                                bg="#F5F7FA", fg="#2C3E50", troughcolor="#E5E9F2",
                                sliderrelief="flat", bd=0, highlightthickness=0)
    threshold_slider.set(VOLUME_THRESHOLD)
    threshold_slider.pack(pady=10, padx=20)

    root.protocol("WM_DELETE_WINDOW", on_closing)

    monitor_thread = threading.Thread(target=volume_monitor_loop, daemon=True)
    monitor_thread.start()
    root.mainloop()