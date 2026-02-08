import os
import subprocess
import shutil
import glob
from datetime import datetime

# Configuration
TEST_BUILD_DIR = "tests/build"
DOCS_DIR = "docs"
AUDIO_DIR = os.path.join(DOCS_DIR, "audio")
INDEX_FILE = os.path.join(DOCS_DIR, "index.md")

def run_demos():
    print(f"Scanning for demos in {TEST_BUILD_DIR}...")
    # Find executables starting with "demo_"
    demos = []
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
        f.write("# PolySynth Audio Tests\n\n")
        f.write(f"**Generated:** {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        f.write("## Audio Artifacts\n\n")
        
        wavs = [a for a in artifacts if a.endswith('.wav')]
        csvs = [a for a in artifacts if a.endswith('.csv')]
        
        if wavs:
            f.write("### Audio Demos\n\n")
            for wav in sorted(wavs):
                f.write(f"#### {wav}\n")
                f.write(f"<audio controls src='audio/{wav}'></audio>\n\n")
        
        if csvs:
            f.write("### Data Demos\n\n")
            for csv in sorted(csvs):
                f.write(f"*   [{csv}](audio/{csv})\n")

        if not wavs and not csvs:
            f.write("No artifacts found.\n")

    print("Done.")

if __name__ == "__main__":
    # Ensure directories exist
    os.makedirs(AUDIO_DIR, exist_ok=True)
    
    run_demos()
    artifacts = collect_artifacts()
    generate_index(artifacts)
