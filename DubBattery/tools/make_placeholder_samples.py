#!/usr/bin/env python3
"""Gera os timbres provisórios (SYN-*.wav) dos pads que ainda não têm sample DM.
Quando os samples de verdade chegarem, troque os WAVs em Samples/ e rode tools/make_samples_header.py.
Uso: python3 tools/make_placeholder_samples.py  (rodar na pasta DubBattery)"""
import os
import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, sosfilt

SR = 44100
rng = np.random.default_rng(1985)


def t(sec):
    return np.arange(int(SR * sec)) / SR


def lp(x, f, order=2):
    return sosfilt(butter(order, f, "low", fs=SR, output="sos"), x)


def hp(x, f, order=2):
    return sosfilt(butter(order, f, "high", fs=SR, output="sos"), x)


def bp(x, lo, hi, order=2):
    return sosfilt(butter(order, [lo, hi], "band", fs=SR, output="sos"), x)


def sweep(f0, f1, rate, sec):
    tt = t(sec)
    f = f1 + (f0 - f1) * np.exp(-tt * rate)
    return np.sin(2 * np.pi * np.cumsum(f) / SR), tt


def crush(x, bits, hold):
    q = 2 ** (bits - 1)
    y = np.round(x * q) / q
    return np.repeat(y[::hold], hold)[: len(x)]


def save(name, x):
    x = x / (np.max(np.abs(x)) + 1e-9) * 0.89
    fade = min(len(x), 256)
    x[-fade:] *= np.linspace(1, 0, fade)
    wavfile.write(os.path.join("Samples", name), SR, (x * 32767).astype(np.int16))


# BUMBO 2: dub grave, senoide caindo de 120 para 50 Hz (estilo steppers)
s, tt = sweep(120, 50, 28, 0.62)
click = lp(rng.standard_normal(len(tt)), 3000) * np.exp(-tt * 300) * 0.3
save("SYN-KICK2.wav", np.tanh(1.6 * (s * np.exp(-tt * 6.5) + click)))

# BUMBO 3: "bip" digital estilo Casio / Sleng Teng
s, tt = sweep(220, 70, 40, 0.18)
save("SYN-KICK3.wav", crush(np.sign(s) * 0.6 * np.exp(-tt * 22), 5, 3))

# CAIXA 2: caixa de rimshot seca, corpo em 190 Hz + esteira
tt = t(0.32)
body = np.sin(2 * np.pi * 190 * tt) * np.exp(-tt * 28) + 0.5 * np.sin(2 * np.pi * 330 * tt) * np.exp(-tt * 40)
noise = bp(rng.standard_normal(len(tt)), 1500, 9000) * np.exp(-tt * 16)
save("SYN-SNARE2.wav", 0.8 * body + noise)

# CAIXA 3: caixa digital curta, granulada
tt = t(0.15)
noise = hp(rng.standard_normal(len(tt)), 900) * np.exp(-tt * 30)
save("SYN-SNARE3.wav", crush(noise + 0.4 * np.sin(2 * np.pi * 240 * tt) * np.exp(-tt * 35), 6, 2))


def tom(f, sec, name):
    s, tt = sweep(f * 1.6, f, 18, sec)
    save(name, s * np.exp(-tt * (4.5 / sec)) + 0.05 * lp(rng.standard_normal(len(tt)), 2000) * np.exp(-tt * 60))


tom(92, 0.75, "SYN-SURDO.wav")
tom(145, 0.55, "SYN-TOM2.wav")
tom(205, 0.45, "SYN-TOM1.wav")

# SHAKER: ruído agudo com ataque suave
tt = t(0.14)
env = np.minimum(tt / 0.018, 1) * np.exp(-np.maximum(tt - 0.018, 0) * 32)
save("SYN-SHAKER.wav", hp(rng.standard_normal(len(tt)), 5500, 4) * env)

# PRATO: seis ondas quadradas desafinadas (receita da 808) + ruído, filtrado no agudo
tt = t(1.9)
metal = sum(np.sign(np.sin(2 * np.pi * f * tt)) for f in (205.3, 304.4, 369.6, 522.7, 540.0, 800.0))
x = hp(bp(metal, 3000, 12000) + 0.6 * rng.standard_normal(len(tt)), 4000, 4)
save("SYN-PRATO.wav", x * (np.exp(-tt * 2.2) + 0.6 * np.exp(-tt * 18)))

# BLOCO: bloco de madeira
tt = t(0.09)
save("SYN-BLOCO.wav", (np.sin(2 * np.pi * 1150 * tt) + 0.45 * np.sin(2 * np.pi * 2630 * tt)) * np.exp(-tt * 70))
print("ok")
