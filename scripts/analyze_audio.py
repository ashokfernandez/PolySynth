#!/usr/bin/env python3
import argparse
import sys
from typing import Tuple

import numpy as np
from scipy.io import wavfile


DEFAULT_MIN_RMS = 0.001
DEFAULT_MAX_RMS = 0.99
DEFAULT_FREQ_TOLERANCE = 5.0
DC_OFFSET_WARN_THRESHOLD = 0.01


def _normalize_audio(data: np.ndarray) -> np.ndarray:
    dtype_kind = data.dtype.kind
    if dtype_kind in {"i", "u"}:
        info = np.iinfo(data.dtype)
        data = data.astype(np.float64)
        if dtype_kind == "u" or info.min == 0:
            midpoint = info.max / 2.0
            scale = midpoint
            data = (data - midpoint) / scale
        else:
            scale = max(abs(info.min), info.max)
            data = data / scale
    else:
        data = data.astype(np.float64)
    return data


def _load_wav(path: str) -> Tuple[int, np.ndarray]:
    rate, data = wavfile.read(path)
    data = _normalize_audio(data)
    if data.ndim > 1:
        data = np.mean(data, axis=1)
    return rate, data


def _dominant_frequency(rate: int, data: np.ndarray) -> float:
    if len(data) == 0:
        return 0.0
    window = np.hanning(len(data))
    spectrum = np.fft.rfft(data * window)
    magnitudes = np.abs(spectrum)
    if len(magnitudes) <= 1:
        return 0.0
    magnitudes[0] = 0.0
    peak_index = int(np.argmax(magnitudes))
    freqs = np.fft.rfftfreq(len(data), d=1.0 / rate)
    return float(freqs[peak_index])


def analyze_audio(path: str, expected_freq: float, freq_tolerance: float,
                  min_rms: float, max_rms: float) -> int:
    rate, data = _load_wav(path)
    if len(data) == 0:
        print("Audio file contains no samples.", file=sys.stderr)
        return 1

    rms = float(np.sqrt(np.mean(np.square(data))))
    if rms < min_rms:
        print(f"RMS {rms:.6f} below minimum {min_rms:.6f}.", file=sys.stderr)
        return 1
    if rms > max_rms:
        print(f"RMS {rms:.6f} above maximum {max_rms:.6f}.", file=sys.stderr)
        return 1

    dc_offset = float(np.mean(data))
    if abs(dc_offset) > DC_OFFSET_WARN_THRESHOLD:
        print(f"Warning: DC offset {dc_offset:.6f} exceeds {DC_OFFSET_WARN_THRESHOLD:.6f}.", file=sys.stderr)

    if expected_freq is not None:
        dominant = _dominant_frequency(rate, data)
        if abs(dominant - expected_freq) > freq_tolerance:
            print(
                f"Dominant frequency {dominant:.2f} Hz differs from expected "
                f"{expected_freq:.2f} Hz by more than {freq_tolerance:.2f} Hz.",
                file=sys.stderr,
            )
            return 1

    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Analyze audio WAV output.")
    parser.add_argument("wav_file", help="Path to WAV file")
    parser.add_argument("--expected-freq", type=float, default=None,
                        help="Verify this frequency is dominant")
    parser.add_argument("--freq-tolerance", type=float, default=DEFAULT_FREQ_TOLERANCE,
                        help=f"Tolerance for frequency match (default: {DEFAULT_FREQ_TOLERANCE})")
    parser.add_argument("--min-rms", type=float, default=DEFAULT_MIN_RMS,
                        help=f"Minimum RMS level required (default: {DEFAULT_MIN_RMS})")
    parser.add_argument("--max-rms", type=float, default=DEFAULT_MAX_RMS,
                        help=f"Maximum RMS level allowed (default: {DEFAULT_MAX_RMS})")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    return analyze_audio(
        args.wav_file,
        args.expected_freq,
        args.freq_tolerance,
        args.min_rms,
        args.max_rms,
    )


if __name__ == "__main__":
    sys.exit(main())
