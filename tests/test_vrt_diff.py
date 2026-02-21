import pytest
from pathlib import Path
from PIL import Image
import subprocess
import sys

def run_vrt_diff(baseline_dir, current_dir, output_dir, tolerance=3.0, max_failed=10):
    cmd = [
        sys.executable,
        "scripts/vrt_diff.py",
        "--baseline-dir", str(baseline_dir),
        "--current-dir", str(current_dir),
        "--output-dir", str(output_dir),
        "--tolerance", str(tolerance),
        "--max-failed-pixels", str(max_failed)
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result

def test_identical_images(tmp_path):
    # Setup
    baseline_dir = tmp_path / "baselines"
    current_dir = tmp_path / "current"
    output_dir = tmp_path / "output"
    baseline_dir.mkdir()
    current_dir.mkdir()
    output_dir.mkdir()

    img = Image.new("RGBA", (64, 64), (255, 0, 0, 255))
    img.save(baseline_dir / "test.png")
    img.save(current_dir / "test.png")

    res = run_vrt_diff(baseline_dir, current_dir, output_dir)
    assert res.returncode == 0
    assert "PASS  test.png (0 differing pixels)" in res.stdout

def test_within_threshold(tmp_path):
    baseline_dir = tmp_path / "baselines"
    current_dir = tmp_path / "current"
    output_dir = tmp_path / "output"
    baseline_dir.mkdir()
    current_dir.mkdir()
    output_dir.mkdir()

    img_b = Image.new("RGBA", (64, 64), (255, 0, 0, 255))
    img_b.save(baseline_dir / "test.png")

    img_c = img_b.copy()
    c_load = img_c.load()
    # Change 5 pixels (max is 10)
    for i in range(5):
        c_load[i, 0] = (0, 255, 0, 255)
    img_c.save(current_dir / "test.png")

    res = run_vrt_diff(baseline_dir, current_dir, output_dir, max_failed=10)
    assert res.returncode == 0
    assert "5 differing pixels, within tolerance" in res.stdout

def test_beyond_threshold(tmp_path):
    baseline_dir = tmp_path / "baselines"
    current_dir = tmp_path / "current"
    output_dir = tmp_path / "output"
    baseline_dir.mkdir()
    current_dir.mkdir()
    output_dir.mkdir()

    img_b = Image.new("RGBA", (64, 64), (255, 0, 0, 255))
    img_b.save(baseline_dir / "test.png")

    img_c = img_b.copy()
    c_load = img_c.load()
    # Change 15 pixels (max is 10)
    for i in range(15):
        c_load[i, 0] = (0, 255, 0, 255)
    img_c.save(current_dir / "test.png")

    res = run_vrt_diff(baseline_dir, current_dir, output_dir, max_failed=10)
    assert res.returncode == 1
    assert "15 differing pixels, max allowed: 10" in res.stdout

def test_missing_baseline(tmp_path):
    baseline_dir = tmp_path / "baselines"
    current_dir = tmp_path / "current"
    output_dir = tmp_path / "output"
    baseline_dir.mkdir()
    current_dir.mkdir()
    output_dir.mkdir()

    img = Image.new("RGBA", (64, 64), (255, 0, 0, 255))
    img.save(current_dir / "test.png")

    res = run_vrt_diff(baseline_dir, current_dir, output_dir)
    assert res.returncode == 1
    assert "Missing baseline" in res.stdout

def test_diff_artifact_red_mask(tmp_path):
    baseline_dir = tmp_path / "baselines"
    current_dir = tmp_path / "current"
    output_dir = tmp_path / "output"
    baseline_dir.mkdir()
    current_dir.mkdir()
    output_dir.mkdir()

    img_b = Image.new("RGBA", (64, 64), (255, 255, 255, 255))
    img_b.save(baseline_dir / "test.png")

    img_c = img_b.copy()
    c_load = img_c.load()
    
    # Change specific pixels
    c_load[10, 10] = (0, 0, 0, 255)
    c_load[20, 20] = (0, 0, 0, 255)
    # Fail due to max_failed=1
    img_c.save(current_dir / "test.png")

    res = run_vrt_diff(baseline_dir, current_dir, output_dir, max_failed=1)
    assert res.returncode == 1
    
    diff_img_path = output_dir / "diff_test.png"
    assert diff_img_path.exists()
    
    diff_img = Image.open(diff_img_path).convert("RGBA")
    d_load = diff_img.load()
    
    assert d_load[10, 10] == (255, 0, 0, 255)
    assert d_load[20, 20] == (255, 0, 0, 255)
    # Unchanged pixel should not be red
    assert d_load[0, 0] != (255, 0, 0, 255)
