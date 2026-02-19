#!/usr/bin/env python3
"""
PolySynth Audio Demo Report Generator

Generates a beautiful, informative HTML page showcasing audio artifacts
from the DSP engine. Runs demo executables and collects generated samples.
"""

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
GALLERY_DIR = os.path.join(DOCS_DIR, "component-gallery")
GALLERY_INDEX_FILE = os.path.join(GALLERY_DIR, "index.html")
WEB_DEMO_DIR = os.path.join(DOCS_DIR, "web-demo")
WEB_DEMO_INDEX_FILE = os.path.join(WEB_DEMO_DIR, "index.html")

# Demo descriptions for rich documentation
# Keys should match the .wav filename (without extension)
DEMO_DESCRIPTIONS = {
    # Original demos
    "demo_engine_saw": {
        "title": "🎵 Sawtooth Oscillator",
        "description": "A pure sawtooth wave demonstrating the fundamental oscillator. The sawtooth is the most harmonically rich basic waveform, containing all harmonics.",
        "technical": "Band-limited sawtooth using BLIT synthesis to prevent aliasing.",
        "listen_for": "Bright, buzzy tone with no aliasing artifacts at high frequencies"
    },
    "demo_render_wav": {
        "title": "🎵 Basic Synthesis",
        "description": "A single sustained note demonstrating the fundamental oscillator and envelope. This is the simplest possible output from the synth engine.",
        "technical": "Single voice, sawtooth wave, full ADSR cycle.",
        "listen_for": "Clean oscillator tone, smooth attack and release"
    },
    "demo_adsr": {
        "title": "📈 Envelope Showcase",
        "description": "Demonstrates the ADSR amplitude envelope generator with various attack, decay, sustain, and release settings.",
        "technical": "Envelope generator modulating amplitude over time.",
        "listen_for": "Different envelope shapes - from snappy to slow"
    },
    "demo_engine_poly": {
        "title": "🎹 Polyphonic Engine",
        "description": "Multiple voices playing simultaneously, showcasing the voice allocation and mixing. Notes are triggered in sequence to test polyphony.",
        "technical": "5-voice polyphony with round-robin allocation.",
        "listen_for": "Clean separation between voices, no clicks or pops"
    },
    "demo_filter_sweep": {
        "title": "🎛️ Filter Sweep",
        "description": "The resonant low-pass filter sweeping from low to high cutoff frequency. Demonstrates the classic 'opening filter' sound.",
        "technical": "24dB/oct resonant LPF with cutoff modulation.",
        "listen_for": "Smooth frequency sweep, resonance peak near cutoff"
    },
    "demo_poly_chords": {
        "title": "🎶 Chord Progressions",
        "description": "Full polyphonic chords demonstrating the synth in a musical context. Multiple notes held together create rich harmonic content.",
        "technical": "Simultaneous voice triggering with harmonic intervals.",
        "listen_for": "Chord clarity, voice blending, no audio artifacts"
    },
    
    # Oscillator demos
    "demo_waveforms": {
        "title": "🔊 Waveform Comparison",
        "description": "Hear all four basic waveforms played in sequence: Sawtooth, Square, Triangle, and Sine. Each has distinct harmonic character.",
        "technical": "Same note (A3) played with each waveform type.",
        "listen_for": "Sawtooth is brightest, Square has hollow quality, Triangle is soft, Sine is pure"
    },
    "demo_pulse_width": {
        "title": "📐 Pulse Width Modulation",
        "description": "A square wave with the pulse width sweeping from thin (10%) to fat (90%). Classic PWM creates movement and thickness.",
        "technical": "Square wave with pulse width modulated over 4 seconds.",
        "listen_for": "Thin nasal tone → fat hollow tone, phasing effect in the middle"
    },
    "demo_osc_mix": {
        "title": "🎚️ Oscillator Mixing",
        "description": "Two oscillators blended together - sawtooth and slightly detuned square. Demonstrates the classic 'thick' synth sound.",
        "technical": "Osc A (saw) + Osc B (square, +5 cents detune), crossfade mix.",
        "listen_for": "Thickening effect when both oscillators play, beating from detune"
    },
    
    # Filter demos
    "demo_filter_resonance": {
        "title": "🌀 Filter Resonance",
        "description": "The same note filtered at increasing resonance (Q) values. Higher resonance creates a peak at the cutoff frequency.",
        "technical": "Low-pass filter at 800Hz, Q values: 1 → 2 → 4 → 8.",
        "listen_for": "Increasingly 'peaky' sound, ringing quality at high Q"
    },
    "demo_filter_envelope": {
        "title": "✨ Filter Envelope",
        "description": "Classic synth plucks and stabs created by the filter envelope. The envelope opens and closes the filter over time.",
        "technical": "Filter cutoff modulated by ADSR: Pluck, Stab, Swell, Long Pluck.",
        "listen_for": "Percussive attacks, the 'bwow' of filter movement"
    },
    
    # LFO demos
    "demo_lfo_vibrato": {
        "title": "〰️ LFO Vibrato",
        "description": "The LFO modulating pitch to create vibrato. Hear all LFO shapes: Sine, Triangle, Square (trill), and Saw.",
        "technical": "5Hz LFO with 3% pitch depth, all 4 waveform shapes.",
        "listen_for": "Smooth vibrato (sine), angular vibrato (square), rising sweep (saw)"
    },
    "demo_lfo_tremolo": {
        "title": "📳 LFO Tremolo",
        "description": "The LFO modulating amplitude for tremolo effect. Rate increases from slow (2Hz) to fast (12Hz) helicopter effect.",
        "technical": "Sine LFO with rate sweep 2Hz→12Hz over 4 seconds.",
        "listen_for": "Slow pulse → medium wobble → fast flutter"
    },
    "demo_lfo_filter_wobble": {
        "title": "🔈 Filter Wobble Bass",
        "description": "Classic 'dubstep wobble' sound - LFO modulating the filter cutoff. Rate increases over time for building tension.",
        "technical": "Sine LFO → filter cutoff (100-3000Hz), rate 1Hz→7Hz.",
        "listen_for": "Rhythmic 'wub wub' sound, increasingly intense"
    },
    
    # Preset demos
    "demo_factory_presets": {
        "title": "🎹 Factory Presets",
        "description": "A musical phrase played with each of the 3 factory presets: Warm Pad (soft), Bright Lead (punchy), Dark Bass (growly).",
        "technical": "C major arpeggio with preset-specific envelope/filter settings.",
        "listen_for": "Distinct character of each preset, same notes sound very different"
    }
}



def run_demos():
    """Execute all demo_* executables in the build directory."""
    print(f"Scanning for demos in {TEST_BUILD_DIR}...")
    demos = []
    
    if os.path.exists(TEST_BUILD_DIR):
        for f in os.listdir(TEST_BUILD_DIR):
            if f.startswith("demo_") and os.access(os.path.join(TEST_BUILD_DIR, f), os.X_OK):
                demos.append(f)
            
    print(f"Found {len(demos)} demos: {demos}")
    
    for demo in demos:
        print(f"Running {demo}...")
        try:
            subprocess.check_call([f"./{demo}"], cwd=TEST_BUILD_DIR)
        except subprocess.CalledProcessError as e:
            print(f"Error running {demo}: {e}")

def collect_artifacts():
    """Copy WAV and CSV files from build directory to docs/audio."""
    print("Collecting artifacts...")
    exts = ['*.wav', '*.csv']
    artifacts = []
    
    if os.path.exists(TEST_BUILD_DIR):
        for ext in exts:
            files = glob.glob(os.path.join(TEST_BUILD_DIR, ext))
            print(f"Found {len(files)} files for extension {ext}")
            for f in files:
                filename = os.path.basename(f)
                dest = os.path.join(AUDIO_DIR, filename)
                shutil.copy2(f, dest)
                artifacts.append(filename)
                print(f"Copied {filename}")
    else:
        print(f"WARNING: Build directory {TEST_BUILD_DIR} does not exist!")
            
    return artifacts

def ensure_gallery_entrypoint():
    """Ensure docs/component-gallery/index.html exists for local browsing."""
    os.makedirs(GALLERY_DIR, exist_ok=True)

    if os.path.exists(GALLERY_INDEX_FILE):
        return

    with open(GALLERY_INDEX_FILE, "w") as f:
        f.write("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Component Gallery (Local)</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            margin: 0;
            padding: 2rem;
            background: #101114;
            color: #e5e7eb;
            line-height: 1.6;
        }
        .card {
            max-width: 760px;
            margin: 2rem auto;
            background: #181b21;
            border: 1px solid #2b3340;
            border-radius: 12px;
            padding: 1.5rem;
        }
        h1 {
            margin-top: 0;
            font-size: 1.5rem;
        }
        code {
            background: #232936;
            padding: 0.15rem 0.4rem;
            border-radius: 4px;
            font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace;
        }
        a {
            color: #7cb6ff;
            text-decoration: none;
        }
        a:hover {
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <div class="card">
        <h1>Component Gallery Not Built Locally Yet</h1>
        <p>This path is populated by CI on GitHub Pages. For local viewing, run:</p>
        <p><code>just gallery-pages-build</code></p>
        <p>Then serve over HTTP (not <code>file://</code>, which browsers block for Storybook scripts):</p>
        <p><code>just gallery-pages-view</code></p>
        <p>Or use live interactive mode with:</p>
        <p><code>just gallery-view</code></p>
        <p><a href="../index.html">← Back to PolySynth Audio Labs</a></p>
    </div>
</body>
</html>
""")


def ensure_web_demo_entrypoint():
    """Ensure docs/web-demo/index.html exists for local browsing."""
    os.makedirs(WEB_DEMO_DIR, exist_ok=True)

    if os.path.exists(WEB_DEMO_INDEX_FILE):
        return

    with open(WEB_DEMO_INDEX_FILE, "w") as f:
        f.write("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PolySynth Web Demo (Local)</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; margin: 0; padding: 2rem; background: #101114; color: #e5e7eb; line-height: 1.6; }
        .card { max-width: 860px; margin: 2rem auto; background: #181b21; border: 1px solid #2b3340; border-radius: 12px; padding: 1.5rem; }
        h1 { margin-top: 0; font-size: 1.5rem; }
        code { background: #232936; padding: 0.15rem 0.4rem; border-radius: 4px; font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace; }
        a { color: #7cb6ff; text-decoration: none; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="card">
        <h1>PolySynth Web Demo Not Built Locally Yet</h1>
        <p>This path is populated by CI on GitHub Pages. For local viewing, run:</p>
        <p><code>cd src/platform/desktop && ./scripts/makedist-web.sh off</code></p>
        <p>Then copy build output into docs:</p>
        <p><code>mkdir -p docs/web-demo && cp -R src/platform/desktop/build-web/. docs/web-demo/</code></p>
        <p><a href="../index.html">← Back to PolySynth Audio Labs</a></p>
    </div>
</body>
</html>
""")

def generate_index(artifacts):
    """Generate a beautiful HTML report page."""
    print(f"Generating {INDEX_FILE}...")
    
    with open(INDEX_FILE, 'w') as f:
        f.write("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PolySynth - Audio Demos & DSP Verification</title>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg-dark: #0a0a0f;
            --bg-card: #12121a;
            --bg-accent: #1a1a25;
            --text-primary: #f0f0f0;
            --text-secondary: #8888a0;
            --accent-blue: #4a9eff;
            --accent-purple: #8b5cf6;
            --accent-green: #22c55e;
            --border-color: #2a2a3a;
            --gradient-1: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            --gradient-2: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
        }
        
        * { box-sizing: border-box; margin: 0; padding: 0; }
        
        body {
            font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: var(--bg-dark);
            color: var(--text-primary);
            line-height: 1.6;
            min-height: 100vh;
        }
        
        .container {
            max-width: 1000px;
            margin: 0 auto;
            padding: 2rem;
        }
        
        /* Hero Header */
        .hero {
            text-align: center;
            padding: 4rem 2rem;
            background: linear-gradient(180deg, rgba(74,158,255,0.1) 0%, transparent 100%);
            border-bottom: 1px solid var(--border-color);
            margin-bottom: 3rem;
        }
        
        .hero h1 {
            font-size: 3rem;
            font-weight: 700;
            background: var(--gradient-1);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
            margin-bottom: 0.5rem;
        }
        
        .hero .tagline {
            font-size: 1.2rem;
            color: var(--text-secondary);
            margin-bottom: 1.5rem;
        }
        
        .hero .meta {
            display: inline-flex;
            align-items: center;
            gap: 0.5rem;
            background: var(--bg-card);
            padding: 0.5rem 1rem;
            border-radius: 50px;
            font-size: 0.85rem;
            color: var(--text-secondary);
            border: 1px solid var(--border-color);
        }
        
        .hero .meta .dot {
            width: 8px;
            height: 8px;
            background: var(--accent-green);
            border-radius: 50%;
            animation: pulse 2s infinite;
        }
        
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        
        /* Back link */
        .back-link {
            display: inline-flex;
            align-items: center;
            gap: 0.5rem;
            color: var(--accent-blue);
            text-decoration: none;
            font-size: 0.9rem;
            margin-bottom: 2rem;
            transition: color 0.2s;
        }
        
        .back-link:hover {
            color: var(--accent-purple);
        }
        
        /* Info Cards */
        .info-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 1.5rem;
            margin-bottom: 3rem;
        }
        
        .info-card {
            background: var(--bg-card);
            border: 1px solid var(--border-color);
            border-radius: 12px;
            padding: 1.5rem;
        }
        
        .info-card h3 {
            font-size: 0.85rem;
            text-transform: uppercase;
            letter-spacing: 0.05em;
            color: var(--text-secondary);
            margin-bottom: 0.5rem;
        }
        
        .info-card p {
            font-size: 1.1rem;
            color: var(--text-primary);
        }

        .info-card .card-link {
            display: inline-block;
            margin-top: 0.75rem;
            color: var(--accent-blue);
            text-decoration: none;
            font-weight: 600;
        }

        .info-card .card-link:hover {
            color: var(--accent-purple);
        }
        
        /* Section Headers */
        .section-header {
            display: flex;
            align-items: center;
            gap: 1rem;
            margin-bottom: 1.5rem;
            padding-bottom: 1rem;
            border-bottom: 1px solid var(--border-color);
        }
        
        .section-header h2 {
            font-size: 1.5rem;
            font-weight: 600;
        }
        
        .section-header .count {
            background: var(--accent-blue);
            color: white;
            font-size: 0.8rem;
            font-weight: 600;
            padding: 0.25rem 0.75rem;
            border-radius: 50px;
        }
        
        /* Demo Cards */
        .demo-card {
            background: var(--bg-card);
            border: 1px solid var(--border-color);
            border-radius: 16px;
            padding: 2rem;
            margin-bottom: 1.5rem;
            transition: border-color 0.2s, transform 0.2s;
        }
        
        .demo-card:hover {
            border-color: var(--accent-blue);
            transform: translateY(-2px);
        }
        
        .demo-card .header {
            display: flex;
            align-items: flex-start;
            gap: 1rem;
            margin-bottom: 1rem;
        }
        
        .demo-card .icon {
            font-size: 2rem;
            line-height: 1;
        }
        
        .demo-card .title-group h3 {
            font-size: 1.25rem;
            font-weight: 600;
            margin-bottom: 0.25rem;
        }
        
        .demo-card .filename {
            font-size: 0.8rem;
            color: var(--text-secondary);
            font-family: 'SF Mono', Monaco, monospace;
            background: var(--bg-accent);
            padding: 0.25rem 0.5rem;
            border-radius: 4px;
        }
        
        .demo-card .description {
            color: var(--text-secondary);
            margin-bottom: 1.5rem;
            font-size: 0.95rem;
        }
        
        .demo-card .details {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 1rem;
            margin-bottom: 1.5rem;
            padding: 1rem;
            background: var(--bg-accent);
            border-radius: 8px;
        }
        
        .demo-card .detail-item label {
            display: block;
            font-size: 0.7rem;
            text-transform: uppercase;
            letter-spacing: 0.05em;
            color: var(--text-secondary);
            margin-bottom: 0.25rem;
        }
        
        .demo-card .detail-item span {
            font-size: 0.9rem;
            color: var(--text-primary);
        }
        
        /* Audio Player */
        .audio-player {
            background: var(--bg-accent);
            border-radius: 12px;
            padding: 1rem;
        }
        
        .audio-player audio {
            width: 100%;
            height: 40px;
        }
        
        audio::-webkit-media-controls-panel {
            background: var(--bg-card);
        }
        
        /* CSV Section */
        .csv-section {
            margin-top: 3rem;
        }
        
        .csv-list {
            display: flex;
            flex-wrap: wrap;
            gap: 0.75rem;
        }
        
        .csv-link {
            display: inline-flex;
            align-items: center;
            gap: 0.5rem;
            background: var(--bg-card);
            border: 1px solid var(--border-color);
            padding: 0.5rem 1rem;
            border-radius: 8px;
            color: var(--text-primary);
            text-decoration: none;
            font-size: 0.9rem;
            transition: border-color 0.2s;
        }
        
        .csv-link:hover {
            border-color: var(--accent-blue);
        }
        
        /* Footer */
        .footer {
            margin-top: 4rem;
            padding-top: 2rem;
            border-top: 1px solid var(--border-color);
            text-align: center;
            color: var(--text-secondary);
            font-size: 0.85rem;
        }
        
        .footer a {
            color: var(--accent-blue);
            text-decoration: none;
        }
        
        /* No artifacts */
        .no-artifacts {
            text-align: center;
            padding: 4rem 2rem;
            background: var(--bg-card);
            border-radius: 16px;
            color: var(--text-secondary);
        }
        
        .no-artifacts .icon {
            font-size: 3rem;
            margin-bottom: 1rem;
        }
    </style>
</head>
<body>
    <div class="hero">
        <h1>🎹 PolySynth Audio Labs</h1>
        <p class="tagline">DSP Verification & Audio Demos</p>
        <div class="meta">
            <span class="dot"></span>
""")
        f.write(f'            <span>Generated {datetime.now().strftime("%B %d, %Y at %H:%M")}</span>\n')
        f.write("""        </div>
    </div>
    
    <div class="container">
        <a href="https://github.com/ashokfernandez/PolySynth" class="back-link">
            ← Back to Repository
        </a>

        <div style="display:flex; gap:0.75rem; flex-wrap:wrap; margin-bottom:1rem;">
            <a class="csv-link" href="#web-demo">🎛️ Web Demo</a>
            <a class="csv-link" href="#audio-labs">🎧 Audio Labs</a>
            <a class="csv-link" href="audio-labs.html">📄 Audio-only Page</a>
            <a class="csv-link" href="component-gallery/index.html">🧩 Component Gallery</a>
        </div>

        <div id="web-demo" class="demo-card" style="margin-bottom:2.25rem;">
            <div class="header">
                <span class="icon">🎛️</span>
                <div class="title-group">
                    <h3>Latest PolySynth Web Demo</h3>
                    <span class="filename">docs/web-demo/index.html</span>
                </div>
            </div>
            <p class="description">Embedded live plugin build published by CI from the latest main branch commit.</p>
            <div style="background:var(--bg-accent); border-radius:12px; padding:0.5rem;">
                <iframe src="web-demo/index.html" title="PolySynth Web Demo" style="width:100%; min-height:820px; border:0; border-radius:8px; background:#111;"></iframe>
            </div>
        </div>

        <div id="audio-labs" class="info-grid">
            <div class="info-card">
                <h3>What is this?</h3>
                <p>Audio samples generated directly from the C++ DSP engine during CI builds. These verify that our oscillators, filters, and envelopes are working correctly.</p>
            </div>
            <div class="info-card">
                <h3>How to use</h3>
                <p>Click play on any sample to hear it. Listen for clean audio without clicks, pops, or distortion. Each demo tests a specific DSP component.</p>
            </div>
            <div class="info-card">
                <h3>Technical details</h3>
                <p>All samples are rendered at 44.1kHz, 16-bit. The synth engine runs headless without any UI or DAW dependencies.</p>
            </div>
            <div class="info-card">
                <h3>UI Component Gallery</h3>
                <p>Browse interactive UI controls and visual regression stories published from Storybook.</p>
                <a class="card-link" href="component-gallery/index.html">Open Component Gallery →</a>
            </div>
        </div>
""")
        
        wavs = sorted([a for a in artifacts if a.endswith('.wav')])
        csvs = sorted([a for a in artifacts if a.endswith('.csv')])
        
        f.write('        <div class="section-header">\n')
        f.write('            <h2>🎧 Audio Demos</h2>\n')
        f.write(f'            <span class="count">{len(wavs)} samples</span>\n')
        f.write('        </div>\n\n')
        
        if wavs:
            for wav in wavs:
                # Extract demo name from filename
                demo_name = wav.replace(".wav", "")
                info = DEMO_DESCRIPTIONS.get(demo_name, {
                    "title": demo_name.replace("_", " ").title(),
                    "description": "Audio sample from the DSP engine.",
                    "technical": "N/A",
                    "listen_for": "Clean audio output"
                })
                
                # Extract emoji if present in title
                icon = info["title"].split()[0] if info["title"][0] in "🎵📈🎹🎛️🎶" else "🔊"
                title = info["title"].lstrip("🎵📈🎹🎛️🎶 ")
                
                f.write(f'''        <div class="demo-card">
            <div class="header">
                <span class="icon">{icon}</span>
                <div class="title-group">
                    <h3>{title}</h3>
                    <span class="filename">{wav}</span>
                </div>
            </div>
            <p class="description">{info["description"]}</p>
            <div class="details">
                <div class="detail-item">
                    <label>Technical</label>
                    <span>{info["technical"]}</span>
                </div>
                <div class="detail-item">
                    <label>Listen for</label>
                    <span>{info["listen_for"]}</span>
                </div>
            </div>
            <div class="audio-player">
                <audio controls preload="metadata">
                    <source src="audio/{wav}" type="audio/wav">
                    Your browser does not support audio playback.
                </audio>
            </div>
        </div>

''')
        else:
            f.write('''        <div class="no-artifacts">
            <div class="icon">🔇</div>
            <p>No audio artifacts found in this build.</p>
            <p>Run the test suite to generate samples.</p>
        </div>
''')
        
        if csvs:
            f.write('''
        <div class="csv-section">
            <div class="section-header">
                <h2>📊 Data Files</h2>
                <span class="count">''' + str(len(csvs)) + ''' files</span>
            </div>
            <div class="csv-list">
''')
            for csv in csvs:
                f.write(f'                <a href="audio/{csv}" class="csv-link">📄 {csv}</a>\n')
            f.write('            </div>\n        </div>\n')
        
        f.write('''
        <div class="footer">
            <p>
                Generated by the PolySynth CI pipeline.<br>
                <a href="https://github.com/ashokfernandez/PolySynth">View source on GitHub</a>
            </p>
        </div>
    </div>
</body>
</html>''')

    print(f"Generated {INDEX_FILE}")

if __name__ == "__main__":
    os.makedirs(AUDIO_DIR, exist_ok=True)
    ensure_gallery_entrypoint()
    ensure_web_demo_entrypoint()
    run_demos()
    artifacts = collect_artifacts()
    generate_index(artifacts)
    shutil.copy2(INDEX_FILE, os.path.join(DOCS_DIR, "audio-labs.html"))
