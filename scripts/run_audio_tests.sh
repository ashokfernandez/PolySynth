#!/usr/bin/env bash
set -euo pipefail

root_dir=$(git rev-parse --show-toplevel)
build_dir="$root_dir/tests/build"
expectations_file="$root_dir/tests/audio_expectations.json"

if [[ ! -d "$build_dir" ]]; then
  mkdir -p "$build_dir"
fi

if [[ ! -f "$build_dir/CMakeCache.txt" ]]; then
  cmake -S "$root_dir/tests" -B "$build_dir"
fi

cmake --build "$build_dir" --target demo_render_wav demo_adsr demo_engine_poly demo_filter_sweep demo_poly_chords

mapfile -t demo_lines < <(
  ROOT_DIR="$root_dir" python - <<'PY'
import json
from pathlib import Path
import os

root_dir = Path(os.environ["ROOT_DIR"])
expectations = json.loads((root_dir / "tests/audio_expectations.json").read_text())
for name, config in sorted(expectations.items()):
    expected = "" if config.get("expected_freq") is None else str(config["expected_freq"])
    min_rms = "" if config.get("min_rms") is None else str(config["min_rms"])
    max_rms = "" if config.get("max_rms") is None else str(config["max_rms"])
    print(f"{name}\t{expected}\t{min_rms}\t{max_rms}")
PY
)

passes=0
failures=0

for line in "${demo_lines[@]}"; do
  IFS=$'\t' read -r demo_name expected_freq min_rms max_rms <<<"$line"

  demo_path="$build_dir/$demo_name"
  if [[ ! -x "$demo_path" ]]; then
    echo "[FAIL] Missing demo executable: $demo_path"
    failures=$((failures + 1))
    continue
  fi

  pre_list=$(mktemp)
  post_list=$(mktemp)
  find "$build_dir" -maxdepth 1 -name "*.wav" -printf "%f\n" | sort > "$pre_list"

  echo "Running $demo_name..."
  set +e
  (cd "$build_dir" && "./$demo_name")
  demo_status=$?
  set -e
  if [[ $demo_status -ne 0 ]]; then
    echo "[FAIL] $demo_name exited with status $demo_status."
    failures=$((failures + 1))
    rm -f "$pre_list" "$post_list"
    continue
  fi

  find "$build_dir" -maxdepth 1 -name "*.wav" -printf "%f\n" | sort > "$post_list"

  new_wavs=$(comm -13 "$pre_list" "$post_list" || true)

  if [[ -z "$new_wavs" && "$demo_name" == "demo_adsr" ]]; then
    csv_path="$build_dir/demo_adsr.csv"
    wav_path="$build_dir/demo_adsr.wav"
    if [[ -f "$csv_path" ]]; then
      ROOT_DIR="$root_dir" python - <<'PY'
import csv
import os
from pathlib import Path
import numpy as np
from scipy.io import wavfile

root_dir = Path(os.environ["ROOT_DIR"])
csv_path = root_dir / "tests/build/demo_adsr.csv"
values = []
with csv_path.open() as handle:
    reader = csv.DictReader(handle)
    for row in reader:
        values.append(float(row["Level"]))

if values:
    data = np.asarray(values, dtype=np.float32)
    wavfile.write(str(root_dir / "tests/build/demo_adsr.wav"), 48000, data)
PY
    fi

    if [[ -f "$wav_path" ]]; then
      new_wavs="$(basename "$wav_path")"
    fi
  fi

  rm -f "$pre_list" "$post_list"

  if [[ -z "$new_wavs" ]]; then
    echo "[FAIL] $demo_name produced no WAV output."
    failures=$((failures + 1))
    continue
  fi

  for wav_file in $new_wavs; do
    wav_path="$build_dir/$wav_file"
    args=("$root_dir/scripts/analyze_audio.py" "$wav_path")
    if [[ -n "$expected_freq" ]]; then
      args+=("--expected-freq" "$expected_freq")
    fi
    if [[ -n "$min_rms" ]]; then
      args+=("--min-rms" "$min_rms")
    fi
    if [[ -n "$max_rms" ]]; then
      args+=("--max-rms" "$max_rms")
    fi

    if python "${args[@]}"; then
      echo "[PASS] $demo_name ($wav_file)"
      passes=$((passes + 1))
    else
      echo "[FAIL] $demo_name ($wav_file)"
      failures=$((failures + 1))
    fi
  done

done

echo "Audio test summary: $passes passed, $failures failed."
if [[ $failures -ne 0 ]]; then
  exit 1
fi
