import os
import subprocess
import shutil
import glob
from datetime import datetime

# Configuration
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
TEST_BUILD_DIR = os.path.join(PROJECT_ROOT, "tests", "build")
DOCS_DIR = os.path.join(PROJECT_ROOT, "docs")
AUDIO_DIR = os.path.join(DOCS_DIR, "audio")
INDEX_FILE = os.path.join(DOCS_DIR, "index.html")

def run_demos():
    print(f"Scanning for demos in {TEST_BUILD_DIR}...")
    # Find executables starting with "demo_"
    demos = []
    if os.path.exists(TEST_BUILD_DIR):
        for f in os.listdir(TEST_BUILD_DIR):
            if f.startswith("demo_") and os.access(os.path.join(TEST_BUILD_DIR, f), os.X_OK):
                demos.append(f)
            
    print(f"Found {len(demos)} demos: {demos}")
    
    generated_files = []
    
    for demo in demos:
        print(f"Running {demo}...")
        try:
            # Run in the build dir so artifacts are created there
            # ./demo_name
            cmd = f"./{demo}"
            subprocess.check_call([cmd], cwd=TEST_BUILD_DIR)
            
            # Look for generated artifacts (.wav, .csv)
            # This is a bit naive, it looks for *any* new files, or we assumes names?
            # Let's assume the demo prints what it generated or we just scan the dir for recent files.
            # Simpler: Scan for all .wav and .csv in build dir
            pass 
        except subprocess.CalledProcessError as e:
            print(f"Error running {demo}: {e}")

def collect_artifacts():
    print("Collecting artifacts...")
    # Extensions to look for
    exts = ['*.wav', '*.csv']
    artifacts = []
    
    if os.path.exists(TEST_BUILD_DIR):
        for ext in exts:
            files = glob.glob(os.path.join(TEST_BUILD_DIR, ext))
            print(f"Found {len(files)} files for extension {ext} in {TEST_BUILD_DIR}")
            for f in files:
                filename = os.path.basename(f)
                dest = os.path.join(AUDIO_DIR, filename)
                shutil.copy2(f, dest)
                artifacts.append(filename)
                print(f"Copied {filename} to {AUDIO_DIR}")
    else:
        print(f"WARNING: Build directory {TEST_BUILD_DIR} does not exist!")
            
    return artifacts

def generate_index(artifacts):
    print(f"Generating {INDEX_FILE}...")
    
def generate_index(artifacts):
    print(f"Generating {INDEX_FILE}...")
    
    with open(INDEX_FILE, 'w') as f:
        f.write("<!DOCTYPE html>\n<html lang='en'>\n<head>\n")
        f.write("<meta charset='UTF-8'>\n<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n")
        f.write("<title>PolySynth - Audio Artifacts & Test Report</title>\n")
        f.write("<style>")
        f.write("body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; max-width: 900px; margin: 0 auto; padding: 2rem; line-height: 1.6; color: #333; } ")
        f.write("header { border-bottom: 2px solid #eaeaea; padding-bottom: 2rem; margin-bottom: 2rem; text-align: center; } ")
        f.write("h1 { margin: 0; color: #111; font-size: 2.5rem; } ")
        f.write(".meta { color: #666; font-size: 0.9rem; margin-top: 0.5rem; } ")
        f.write(".section-title { border-bottom: 1px solid #eee; padding-bottom: 0.5rem; margin-top: 3rem; color: #444; } ")
        f.write(".artifact { background: #f9f9f9; padding: 1.5rem; border-radius: 8px; margin-bottom: 1.5rem; border: 1px solid #eee; } ")
        f.write(".artifact h4 { margin-top: 0; margin-bottom: 0.5rem; color: #222; } ")
        f.write("audio { width: 100%; margin-top: 0.5rem; } ")
        f.write("a { color: #0066cc; text-decoration: none; } a:hover { text-decoration: underline; } ")
        f.write("</style>\n")
        f.write("</head>\n<body>\n")
        
        f.write("<header>\n")
        f.write("<h1>PolySynth Audio Labs</h1>\n")
        f.write(f"<p class='meta'>Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>\n")
        f.write("<p>Welcome to the <strong>PolySynth</strong> automated test report. This page contains audio artifacts generated directly from our C++ DSP engine during the latest CI build. These samples verify the correctness of our Oscillators, Filters, and Envelopes.</p>\n")
        f.write("<p><a href='https://github.com/ashokfernandez/PolySynth'>&larr; Back to Repository</a></p>\n")
        f.write("</header>\n")

        f.write("<section>\n")
        f.write("<h2 class='section-title'>Latest Audio Artifacts</h2>\n")
        
        wavs = [a for a in artifacts if a.endswith('.wav')]
        csvs = [a for a in artifacts if a.endswith('.csv')]
        
        if wavs:
            for wav in sorted(wavs):
                clean_name = wav.replace("demo_", "").replace(".wav", "").replace("_", " ").title()
                f.write(f"<div class='artifact'>\n")
                f.write(f"<h4>{clean_name} ({wav})</h4>\n")
                f.write(f"<audio controls src='audio/{wav}' preload='metadata'></audio>\n")
                f.write("</div>\n")
        elif not csvs:
             f.write("<p>No audio artifacts found in this build.</p>\n")

        if csvs:
            f.write("<h2 class='section-title'>Data Dumps (CSV)</h2>\n")
            f.write("<ul>\n")
            for csv in sorted(csvs):
                 f.write(f"<li><a href='audio/{csv}'>{csv}</a></li>\n")
            f.write("</ul>\n")
            
        f.write("</section>\n")
            
        f.write("</body>\n</html>")

    print(f"Generated {INDEX_FILE}")

if __name__ == "__main__":
    # Ensure directories exist
    os.makedirs(AUDIO_DIR, exist_ok=True)
    
    run_demos()
    artifacts = collect_artifacts()
    generate_index(artifacts)
