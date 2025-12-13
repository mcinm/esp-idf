#!/usr/bin/env python3

import wave
import struct
import os
import socket

dane_raw = []

sample_rate = 12000  # Hz
num_channels = 1     # Mono
sample_width = 3     # 24-bit = 3 bajty
wav_filename = "dane.wav"
wzmocnienie = 90


def generate_wav():
    print(f"Samples: {len(dane_raw)}")

    # Konwersja wartości liczbowych na binarne PCM (little-endian, signed 32-bit)
    pcm_data = bytearray()
    for value in dane_raw:
        value = value >> 8
        value = value * wzmocnienie
        if value > 8388607: value = 8388607
        elif value < -8388607: value = -8388607
        b = struct.pack("<i", value)
        pcm_data.extend(b[:3])

    print(f"RAW data PCM: {len(pcm_data)} bajtów")

    # Konwersja wartości liczbowych na binarne PCM (little-endian, signed 32-bit)
    with wave.open(wav_filename, "wb") as wav_file:
        wav_file.setnchannels(num_channels)
        wav_file.setsampwidth(sample_width)
        wav_file.setframerate(sample_rate)
        wav_file.writeframes(pcm_data)

    # Sprawdzenie rozmiaru zapisanego pliku
    file_size = os.path.getsize(wav_filename)
    print(f"WAV file: {wav_filename}")
    print(f"FIle size: {file_size} bajtów")

    # Sprawdzamy, czy nagłówek WAV się zgadza
    with wave.open(wav_filename, "rb") as wav_file:
        print(f"WAV Info: {wav_file.getnchannels()} kanał(y), {wav_file.getsampwidth() * 8}-bit, {wav_file.getframerate()} Hz")


hostname = "192.168.xx.xx"
PORT = 8876

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind((hostname, PORT))

print(f"Nasłuchiwanie na {hostname}:{PORT}")

try:
    while True:
        data, addr = s.recvfrom(2048)
        if data:
            count = len(data) // 4
            values = struct.unpack('<' + 'i'*count, data)

            for v in values:
                dane_raw.append(v)

except KeyboardInterrupt:
    print("Zatrzymano serwer")
    if len(dane_raw) != 0: generate_wav()
finally:
    s.close()
