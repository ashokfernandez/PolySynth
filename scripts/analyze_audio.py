#!/usr/bin/env python3
import argparse
import array
import cmath
import math
import sys
import wave
import statistics

try:
    import numpy as np
except ImportError:  # pragma: no cover - optional acceleration
    np = None


def read_wav(path):
    with wave.open(path, "rb") as wav_file:
        if wav_file.getsampwidth() != 2:
            raise ValueError(f"Unsupported sample width {wav_file.getsampwidth()} bytes")
        if wav_file.getnchannels() != 1:
            raise ValueError("Only mono WAV files are supported")
        sample_rate = wav_file.getframerate()
        frames = wav_file.readframes(wav_file.getnframes())
    sample_array = array.array("h")
    sample_array.frombytes(frames)
    if sys.byteorder != "little":
        sample_array.byteswap()
    samples = [sample / 32768.0 for sample in sample_array]
    return sample_rate, samples


def compute_rms(samples):
    if not samples:
        return 0.0
    accum = math.fsum(sample * sample for sample in samples)
    return math.sqrt(accum / len(samples))


def next_pow_two(value):
    return 1 << (value - 1).bit_length()


def fft_inplace(buffer):
    n = len(buffer)
    j = 0
    for i in range(1, n):
        bit = n >> 1
        while j & bit:
            j ^= bit
            bit >>= 1
        j ^= bit
        if i < j:
            buffer[i], buffer[j] = buffer[j], buffer[i]

    length = 2
    while length <= n:
        angle = -2j * math.pi / length
        wlen = cmath.exp(angle)
        for i in range(0, n, length):
            w = 1 + 0j
            half = length // 2
            for j in range(i, i + half):
                u = buffer[j]
                v = buffer[j + half] * w
                buffer[j] = u + v
                buffer[j + half] = u - v
                w *= wlen
        length *= 2


def rfft_magnitudes(samples, sample_rate):
    if np is not None:
        window = np.hanning(len(samples))
        spectrum = np.fft.rfft(np.array(samples, dtype=np.float64) * window)
        magnitudes = np.abs(spectrum).astype(float).tolist()
        freqs = np.fft.rfftfreq(len(samples), d=1.0 / sample_rate).astype(float).tolist()
        return freqs, magnitudes

    if not samples:
        return [], []
    n = next_pow_two(len(samples))
    windowed = []
    for i in range(len(samples)):
        if len(samples) > 1:
            window = 0.5 * (1.0 - math.cos(2.0 * math.pi * i / (len(samples) - 1)))
        else:
            window = 1.0
        windowed.append(samples[i] * window)
    if n > len(windowed):
        windowed.extend([0.0] * (n - len(windowed)))
    buffer = [complex(value, 0.0) for value in windowed]
    fft_inplace(buffer)
    half = n // 2
    magnitudes = [abs(buffer[i]) for i in range(half + 1)]
    freqs = [i * sample_rate / n for i in range(half + 1)]
    return freqs, magnitudes


def analyze_frequencies(samples, sample_rate, expected_freqs):
    if not expected_freqs:
        return []
    if not samples:
        return ["No samples to analyze for frequency content."]

    freqs, magnitudes = rfft_magnitudes(samples, sample_rate)
    if not magnitudes:
        return ["Spectrum is empty (no magnitudes)."]
    max_mag = max(magnitudes)
    if max_mag == 0.0:
        return ["Spectrum is silent (zero magnitude)."]

    noise_floor = statistics.median(magnitudes)
    if noise_floor <= 0.0:
        noise_floor = max_mag * 1e-6

    issues = []
    for expected in expected_freqs:
        tolerance = max(5.0, expected * 0.02)
        band_indices = [i for i, freq in enumerate(freqs) if abs(freq - expected) <= tolerance]
        if not band_indices:
            issues.append(f"Expected frequency {expected:.2f} Hz not found (no bins in range).")
            continue
        band_peak = max(magnitudes[i] for i in band_indices)
        band_freq = freqs[max(band_indices, key=lambda i: magnitudes[i])]
        if band_peak < noise_floor * 20.0:
            issues.append(
                "Expected frequency {:.2f} Hz too weak (peak {:.6f} at {:.2f} Hz).".format(
                    expected, band_peak, band_freq
                )
            )
    return issues


def main():
    parser = argparse.ArgumentParser(description="Analyze WAV files for frequency and RMS checks.")
    parser.add_argument("wav_path", help="Path to WAV file to analyze.")
    parser.add_argument(
        "--expected-freq",
        type=float,
        action="append",
        default=[],
        help="Expected frequency in Hz (can be supplied multiple times).",
    )
    parser.add_argument("--min-rms", type=float, default=None, help="Minimum RMS threshold.")
    parser.add_argument("--max-rms", type=float, default=None, help="Maximum RMS threshold.")
    args = parser.parse_args()

    try:
        sample_rate, samples = read_wav(args.wav_path)
    except Exception as exc:
        print(f"ERROR: Failed to read WAV: {exc}", file=sys.stderr)
        return 1

    rms = compute_rms(samples)
    if args.min_rms is not None and rms < args.min_rms:
        print(f"ERROR: RMS {rms:.6f} below minimum {args.min_rms:.6f}", file=sys.stderr)
        return 1
    if args.max_rms is not None and rms > args.max_rms:
        print(f"ERROR: RMS {rms:.6f} above maximum {args.max_rms:.6f}", file=sys.stderr)
        return 1

    issues = analyze_frequencies(samples, sample_rate, args.expected_freq)
    if issues:
        for issue in issues:
            print(f"ERROR: {issue}", file=sys.stderr)
        return 1

    print(f"PASS: RMS={rms:.6f}, sample_rate={sample_rate} Hz")
    return 0


if __name__ == "__main__":
    sys.exit(main())
