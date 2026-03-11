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

# Embedded-configuration paths (Pico-equivalent: float, 4 voices, no FX).
EMBEDDED_BUILD_DIR = os.path.join(PROJECT_ROOT, "tests", "build")
EMBEDDED_GOLDEN_DIR = os.path.join(PROJECT_ROOT, "tests", "golden_embedded")
EMBEDDED_SUFFIX = "_embedded"

# ARM cross-compiled paths (real ARM float via QEMU user-mode).
ARM_BUILD_DIR = os.path.join(PROJECT_ROOT, "tests", "build_arm")
ARM_GOLDEN_DIR = os.path.join(PROJECT_ROOT, "tests", "golden_arm")


def find_demos(build_dir, suffix=""):
    """Find demo executables in build_dir, optionally filtering by suffix."""
    demos = []
    if os.path.isdir(build_dir):
        for entry in os.listdir(build_dir):
            if entry.startswith("demo_"):
                if suffix and not entry.endswith(suffix):
                    continue
                if suffix == "" and EMBEDDED_SUFFIX in entry:
                    continue  # Skip embedded demos in normal mode
                path = os.path.join(build_dir, entry)
                if os.access(path, os.X_OK) and os.path.isfile(path):
                    demos.append(entry)
    return sorted(demos)


def run_demos(demos, build_dir):
    if not demos:
        raise RuntimeError(f"No demo executables found in {build_dir}")
    for demo in demos:
        subprocess.check_call([f"./{demo}"], cwd=build_dir)


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


def generate_golden(build_dir=TEST_BUILD_DIR, golden_dir=GOLDEN_DIR, suffix=""):
    os.makedirs(golden_dir, exist_ok=True)
    demos = find_demos(build_dir, suffix)
    run_demos(demos, build_dir)
    wavs = list_wavs(build_dir)
    if not wavs:
        raise RuntimeError("No WAV files produced by demos.")
    for wav in wavs:
        shutil.copy2(os.path.join(build_dir, wav), os.path.join(golden_dir, wav))
        print(f"Golden master written: {wav}")
    if not suffix:
        for alias, target in ALIAS_MAP.items():
            target_path = os.path.join(build_dir, target)
            if os.path.exists(target_path):
                shutil.copy2(target_path, os.path.join(golden_dir, alias))
                print(f"Golden master written (alias): {alias} -> {target}")


def verify_golden(tolerance, build_dir=TEST_BUILD_DIR, golden_dir=GOLDEN_DIR, suffix=""):
    demos = find_demos(build_dir, suffix)
    run_demos(demos, build_dir)
    if not os.path.isdir(golden_dir):
        raise RuntimeError(f"Golden directory missing: {golden_dir}")

    golden_wavs = list_wavs(golden_dir)
    if not golden_wavs:
        raise RuntimeError("No golden WAV files found.")

    failures = []
    for wav in golden_wavs:
        golden_path = os.path.join(golden_dir, wav)
        if suffix:
            compare_name = wav  # Embedded WAV names match golden names
        else:
            compare_name = ALIAS_MAP.get(wav, wav)
        test_path = os.path.join(build_dir, compare_name)
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

    extra_wavs = set(list_wavs(build_dir)) - set(golden_wavs)
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
    parser.add_argument(
        "--embedded",
        action="store_true",
        help="Use embedded (Pico) configuration demos and golden directory.",
    )
    parser.add_argument(
        "--arm",
        action="store_true",
        help="Use ARM cross-compiled demos and golden directory (QEMU user-mode).",
    )
    parser.add_argument(
        "--build-dir",
        type=str,
        default=None,
        help="Override build directory path.",
    )
    parser.add_argument(
        "--golden-dir",
        type=str,
        default=None,
        help="Override golden reference directory path.",
    )
    args = parser.parse_args()

    if not args.generate and not args.verify:
        print("ERROR: Specify --generate or --verify.", file=sys.stderr)
        return 2

    # Determine directories and suffix.
    if args.arm:
        build_dir = args.build_dir or ARM_BUILD_DIR
        golden_dir = args.golden_dir or ARM_GOLDEN_DIR
        suffix = EMBEDDED_SUFFIX
    elif args.embedded:
        build_dir = args.build_dir or EMBEDDED_BUILD_DIR
        golden_dir = args.golden_dir or EMBEDDED_GOLDEN_DIR
        suffix = EMBEDDED_SUFFIX
    else:
        build_dir = args.build_dir or TEST_BUILD_DIR
        golden_dir = args.golden_dir or GOLDEN_DIR
        suffix = ""

    try:
        if args.generate:
            generate_golden(build_dir, golden_dir, suffix)
        if args.verify:
            verify_golden(args.tolerance, build_dir, golden_dir, suffix)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
