#!/usr/bin/env python3
import argparse
import json
import math
import os
import sys
from pathlib import Path
from PIL import Image

def pixel_distance(p1, p2):
    """Euclidean distance in RGBA space, normalized to 0-100%"""
    r = (p1[0] - p2[0]) ** 2
    g = (p1[1] - p2[1]) ** 2
    b = (p1[2] - p2[2]) ** 2
    a = (p1[3] - p2[3]) ** 2 if len(p1) > 3 and len(p2) > 3 else 0
    max_dist = math.sqrt(255**2 * 4)  # Maximum possible distance
    return (math.sqrt(r + g + b + a) / max_dist) * 100.0

def main():
    parser = argparse.ArgumentParser(description="VRT Diff Engine")
    parser.add_argument("--baseline-dir", required=True)
    parser.add_argument("--current-dir", required=True)
    parser.add_argument("--output-dir", default="tests/Visual/output")
    parser.add_argument("--tolerance", type=float)
    parser.add_argument("--max-failed-pixels", type=int)
    parser.add_argument("--config", default="tests/Visual/vrt_config.json")
    
    args = parser.parse_args()

    config = {}
    if os.path.exists(args.config):
        with open(args.config, "r") as f:
            config = json.load(f)
            
    tolerance = args.tolerance if args.tolerance is not None else config.get("tolerance", 3.0)
    max_failed = args.max_failed_pixels if args.max_failed_pixels is not None else config.get("max_failed_pixels", 10)
    # crop_top_px is expressed in logical (1x) pixels and scaled to match each
    # image's actual capture resolution. 28 = macOS standard title bar height.
    crop_top_logical_px = config.get("crop_top_px", 0)

    print(f"Using tolerance: {tolerance}% (from {'CLI' if args.tolerance is not None else 'config'})")
    print(f"Using max_failed_pixels: {max_failed} (from {'CLI' if args.max_failed_pixels is not None else 'config'})")
    if crop_top_logical_px:
        print(f"Cropping top {crop_top_logical_px}px logical (window chrome exclusion)")

    baseline_dir = Path(args.baseline_dir)
    current_dir = Path(args.current_dir)
    output_dir = Path(args.output_dir)
    
    output_dir.mkdir(parents=True, exist_ok=True)
    
    all_passed = True
    failing_components = []
    
    for current_img_path in current_dir.glob("*.png"):
        img_name = current_img_path.name
        baseline_img_path = baseline_dir / img_name
        
        if not baseline_img_path.exists():
            print(f"FAIL  {img_name} (Missing baseline)")
            all_passed = False
            failing_components.append(img_name)
            continue
            
        try:
            current_img = Image.open(current_img_path).convert("RGBA")
            baseline_img = Image.open(baseline_img_path).convert("RGBA")

            # Detect pixel ratio and crop title bar scaled to capture resolution.
            # Baseline is the reference size; current may be at a different scale
            # (e.g. 2x Retina locally vs 1x on CI).
            if baseline_img.width > 0 and current_img.width > 0:
                scale = current_img.width / baseline_img.width
            else:
                scale = 1.0

            if crop_top_logical_px > 0:
                # Crop baseline (always at its native resolution)
                bw, bh = baseline_img.size
                b_crop = round(crop_top_logical_px)
                baseline_img = baseline_img.crop((0, b_crop, bw, bh))
                # Crop current scaled to its capture resolution
                cw, ch = current_img.size
                c_crop = round(crop_top_logical_px * scale)
                current_img = current_img.crop((0, c_crop, cw, ch))

            # Normalise both images to the smaller resolution so comparison is
            # always at the lower DPI (avoids upscaling artefacts). This makes
            # baselines generated on a 2x Retina machine work on a 1x CI runner.
            if current_img.size != baseline_img.size:
                target_w = min(current_img.width, baseline_img.width)
                target_h = min(current_img.height, baseline_img.height)
                target = (target_w, target_h)
                if current_img.size != target:
                    current_img = current_img.resize(target, Image.LANCZOS)
                if baseline_img.size != target:
                    baseline_img = baseline_img.resize(target, Image.LANCZOS)
        except Exception as e:
            print(f"FAIL  {img_name} (Error loading images: {e})")
            all_passed = False
            failing_components.append(img_name)
            continue

        if current_img.size != baseline_img.size:
            print(f"FAIL  {img_name} (Dimension mismatch after resize: baseline={baseline_img.width}x{baseline_img.height} current={current_img.width}x{current_img.height})")
            all_passed = False
            failing_components.append(img_name)
            continue
            
        diff_count = 0
        w, h = current_img.size
        
        c_load = current_img.load()
        b_load = baseline_img.load()
        
        diff_pixels = []
        for x in range(w):
            for y in range(h):
                c_px = c_load[x,y]
                b_px = b_load[x,y]
                if c_px == b_px:
                    continue
                dist = pixel_distance(c_px, b_px)
                if dist > tolerance:
                    diff_count += 1
                    diff_pixels.append((x, y))

        if diff_count > max_failed:
            print(f"FAIL  {img_name} ({diff_count} differing pixels, max allowed: {max_failed})")
            all_passed = False
            failing_components.append(img_name)
            
            # Create diff artifact
            diff_img = baseline_img.convert("L").convert("RGBA")
            d_load = diff_img.load()
            for x, y in diff_pixels:
                d_load[x, y] = (255, 0, 0, 255)
            diff_img.save(output_dir / f"diff_{img_name}")
        else:
            if diff_count == 0:
                print(f"PASS  {img_name} (0 differing pixels)")
            else:
                print(f"PASS  {img_name} ({diff_count} differing pixels, within tolerance)")

    # Check for orphan baselines
    for baseline_img_path in baseline_dir.glob("*.png"):
        img_name = baseline_img_path.name
        if not (current_dir / img_name).exists():
            print(f"WARN  Orphan baseline: {img_name} (no matching current image)")
            
    if not all_passed:
        print("\nFailing components:", ", ".join(failing_components))
        sys.exit(1)
        
    sys.exit(0)

if __name__ == "__main__":
    main()
