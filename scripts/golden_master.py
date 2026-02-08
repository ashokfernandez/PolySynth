#!/usr/bin/env python3
import argparse
import math
import os
import shutil
import subprocess
import sys
import wave

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
TEST_BUILD_DIR = os.path.join(PROJECT_ROOT, "tests", "build")
GOLDEN_DIR = os.path.join(PROJECT_ROOT, "tests", "golden")
ALIAS_MAP = {
    "demo_render_wav.wav": "demo_engine_saw.wav",
}


def find_demos():
    demos = []
    if os.path.isdir(TEST_BUILD_DIR):
        for entry in os.listdir(TEST_BUILD_DIR):
            if entry.startswith("demo_"):
                path = os.path.join(TEST_BUILD_DIR, entry)
                if os.access(path, os.X_OK) and os.path.isfile(path):
                    demos.append(entry)
    return sorted(demos)


def run_demos(demos):
    if not demos:
        raise RuntimeError(f"No demo executables found in {TEST_BUILD_DIR}")
    for demo in demos:
        subprocess.check_call([f"./{demo}"], cwd=TEST_BUILD_DIR)


def list_wavs(directory):
    return sorted(
        f for f in os.listdir(directory) if f.endswith(".wav") and f.startswith("demo_")
    )


def read_wav(path):
    with wave.open(path, "rb") as wav_file:
        if wav_file.getsampwidth() != 2:
            raise ValueError(f"Unsupported sample width {wav_file.getsampwidth()} bytes")
        if wav_file.getnchannels() != 1:
            raise ValueError("Only mono WAV files are supported")
        sample_rate = wav_file.getframerate()
        frames = wav_file.readframes(wav_file.getnframes())
    samples = [
        (int.from_bytes(frames[i : i + 2], byteorder="little", signed=True) / 32768.0)
        for i in range(0, len(frames), 2)
    ]
    return sample_rate, samples


def rms_diff(a, b):
    if len(a) != len(b):
        raise ValueError(f"Sample length mismatch ({len(a)} vs {len(b)})")
    if not a:
        return 0.0
    accum = 0.0
    for x, y in zip(a, b):
        diff = x - y
        accum += diff * diff
    return math.sqrt(accum / len(a))


def generate_golden():
    os.makedirs(GOLDEN_DIR, exist_ok=True)
    demos = find_demos()
    run_demos(demos)
    wavs = list_wavs(TEST_BUILD_DIR)
    if not wavs:
        raise RuntimeError("No WAV files produced by demos.")
    for wav in wavs:
        shutil.copy2(os.path.join(TEST_BUILD_DIR, wav), os.path.join(GOLDEN_DIR, wav))
        print(f"Golden master written: {wav}")
    for alias, target in ALIAS_MAP.items():
        target_path = os.path.join(TEST_BUILD_DIR, target)
        if os.path.exists(target_path):
            shutil.copy2(target_path, os.path.join(GOLDEN_DIR, alias))
            print(f"Golden master written (alias): {alias} -> {target}")


def verify_golden(tolerance):
    demos = find_demos()
    run_demos(demos)
    if not os.path.isdir(GOLDEN_DIR):
        raise RuntimeError(f"Golden directory missing: {GOLDEN_DIR}")

    golden_wavs = list_wavs(GOLDEN_DIR)
    if not golden_wavs:
        raise RuntimeError("No golden WAV files found.")

    failures = []
    for wav in golden_wavs:
        golden_path = os.path.join(GOLDEN_DIR, wav)
        compare_name = ALIAS_MAP.get(wav, wav)
        test_path = os.path.join(TEST_BUILD_DIR, compare_name)
        if not os.path.exists(test_path):
            failures.append(f"Missing generated WAV: {compare_name}")
            continue
        try:
            golden_rate, golden_samples = read_wav(golden_path)
            test_rate, test_samples = read_wav(test_path)
        except Exception as exc:
            failures.append(f"Failed to read WAV {wav}: {exc}")
            continue
        if golden_rate != test_rate:
            failures.append(
                f"Sample rate mismatch for {wav} ({golden_rate} vs {test_rate})"
            )
            continue
        try:
            diff = rms_diff(golden_samples, test_samples)
        except Exception as exc:
            failures.append(f"RMS comparison failed for {wav}: {exc}")
            continue
        if diff > tolerance:
            failures.append(f"{wav} RMS diff {diff:.6f} exceeds tolerance {tolerance:.6f}")
        else:
            print(f"{wav}: RMS diff {diff:.6f} (tolerance {tolerance:.6f})")

    extra_wavs = set(list_wavs(TEST_BUILD_DIR)) - set(golden_wavs)
    for wav in sorted(extra_wavs):
        print(f"Warning: Extra WAV generated (not in golden set): {wav}")

    if failures:
        for failure in failures:
            print(f"ERROR: {failure}", file=sys.stderr)
        raise RuntimeError("Golden master verification failed.")


def main():
    parser = argparse.ArgumentParser(description="Generate and verify golden master WAVs.")
    parser.add_argument("--generate", action="store_true", help="Generate golden masters.")
    parser.add_argument("--verify", action="store_true", help="Verify against golden masters.")
    parser.add_argument(
        "--tolerance",
        type=float,
        default=0.001,
        help="RMS tolerance threshold (default: 0.001).",
    )
    args = parser.parse_args()

    if not args.generate and not args.verify:
        print("ERROR: Specify --generate or --verify.", file=sys.stderr)
        return 2

    try:
        if args.generate:
            generate_golden()
        if args.verify:
            verify_golden(args.tolerance)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
