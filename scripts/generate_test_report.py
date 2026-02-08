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
            for f in glob.glob(os.path.join(TEST_BUILD_DIR, ext)):
                filename = os.path.basename(f)
                dest = os.path.join(AUDIO_DIR, filename)
                shutil.copy2(f, dest)
                artifacts.append(filename)
                print(f"Copied {filename} to {AUDIO_DIR}")
            
    return artifacts

def generate_index(artifacts):
    print(f"Generating {INDEX_FILE}...")
    
    with open(INDEX_FILE, 'w') as f:
        f.write("<!DOCTYPE html>\n<html>\n<head>\n<title>PolySynth Audio Tests</title>\n")
        f.write("<style>body { font-family: sans-serif; max-width: 800px; margin: 2rem auto; padding: 0 1rem; } ")
        f.write("h1 { border-bottom: 2px solid #eee; padding-bottom: 0.5rem; } ")
        f.write(".artifact { margin-bottom: 1.5rem; } audio { width: 100%; }</style>\n")
        f.write("</head>\n<body>\n")
        f.write("<h1>PolySynth Audio Tests</h1>\n")
        f.write(f"<p><strong>Generated:</strong> {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>\n")
        f.write("<h2>Audio Artifacts</h2>\n")
        
        wavs = [a for a in artifacts if a.endswith('.wav')]
        csvs = [a for a in artifacts if a.endswith('.csv')]
        
        if wavs:
            f.write("<h3>Audio Demos</h3>\n")
            for wav in sorted(wavs):
                f.write(f"<div class='artifact'><h4>{wav}</h4>\n")
                f.write(f"<audio controls src='audio/{wav}'></audio></div>\n")
        
        if csvs:
            f.write("<h3>Data Demos</h3>\n")
            f.write("<ul>\n")
            for csv in sorted(csvs):
                f.write(f"<li><a href='audio/{csv}'>{csv}</a></li>\n")
            f.write("</ul>\n")

        if not wavs and not csvs:
            f.write("<p>No artifacts found.</p>\n")
            
        f.write("</body>\n</html>")

    print("Done.")

if __name__ == "__main__":
    # Ensure directories exist
    os.makedirs(AUDIO_DIR, exist_ok=True)
    
    run_demos()
    artifacts = collect_artifacts()
    generate_index(artifacts)
