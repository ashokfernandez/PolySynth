import os
import subprocess
import time
import sys

def test_ui_render():
    # Determine the path to the executable
    # Assumes running from project root
    app_path = os.path.join("build_desktop", "out", "Debug", "PolySynth.app", "Contents", "MacOS", "PolySynth")
    
    if not os.path.exists(app_path):
        print(f"Error: App not found at {app_path}")
        print("Please build the desktop app first using ./rebuild_and_run.sh")
        sys.exit(1)

    env = os.environ.copy()
    env["POLYSYNTH_TEST_UI"] = "1"

    print(f"Launching {app_path} with POLYSYNTH_TEST_UI=1...")
    
    try:
        # Run the process
        # We need to capture stdout to check for the success message
        process = subprocess.Popen(
            [app_path], 
            env=env, 
            stdout=subprocess.PIPE, 
            stderr=subprocess.PIPE, 
            text=True
        )
        
        start_time = time.time()
        timeout = 15  # seconds
        
        while True:
            # Check for timeout
            if time.time() - start_time > timeout:
                print("Error: Test timed out")
                process.terminate()
                try:
                    stdout, stderr = process.communicate(timeout=2)
                    print("STDOUT (partial):", stdout)
                    print("STDERR (partial):", stderr)
                except:
                    pass
                sys.exit(1)
            
            # Check if process has exited
            retcode = process.poll()
            if retcode is not None:
                stdout, stderr = process.communicate()
                
                # Check for success message in stdout
                if "TEST_PASS: UI Loaded" in stdout:
                    print("SUCCESS: UI Render Test Passed")
                    sys.exit(0)
                else:
                    print(f"FAILURE: Process exited with code {retcode} but success message not found.")
                    print("STDOUT:", stdout)
                    print("STDERR:", stderr)
                    sys.exit(1)
            
            time.sleep(0.5)

    except Exception as e:
        print(f"An exception occurred: {e}")
        sys.exit(1)

if __name__ == "__main__":
    test_ui_render()
