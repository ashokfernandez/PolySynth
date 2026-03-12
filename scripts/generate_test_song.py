#!/usr/bin/env python3
"""Generate a test song header for PolySynth Pico.

Creates a funky bass+lead+brass arrangement inspired by
"Don't Stop 'Til You Get Enough" — Bb major, 120 BPM.

Outputs a C++ constexpr header file.
"""

import sys


def note(name):
    """Convert note name to MIDI number. e.g. 'Bb2' -> 46"""
    notes = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}
    i = 0
    base = notes[name[i]]
    i += 1
    if i < len(name) and name[i] == 'b':
        base -= 1
        i += 1
    elif i < len(name) and name[i] == '#':
        base += 1
        i += 1
    octave = int(name[i])
    return base + (octave + 1) * 12


# Tempo
BPM = 120.0
BEAT = 60000.0 / BPM   # 500ms
EIGHTH = BEAT / 2       # 250ms
SIXTEENTH = BEAT / 4    # 125ms

events = []  # (abs_ms, type, note, velocity)

def note_on(time_ms, n, vel=100):
    events.append((time_ms, "NoteOn", n, vel))

def note_off(time_ms, n):
    events.append((time_ms, "NoteOff", n, 0))

def bass_note(start, n, duration, vel=100):
    note_on(start, n, vel)
    note_off(start + duration, n)


# ── Bass line (Bb2=46) — 16 bars, 2-bar repeating pattern ──────────────
# Funky pumping eighths with chromatic walk-ups
Bb2 = note('Bb2')  # 46
C3  = note('C3')   # 48
D3  = note('D3')   # 50
Eb3 = note('Eb3')  # 51
F3  = note('F3')   # 53

def bass_pattern(bar_start):
    """2-bar bass pattern: pumping Bb2 eighths with walk-up at end of bar 2."""
    t = bar_start
    dur = EIGHTH * 0.8  # slightly staccato

    # Bar 1: straight eighths on Bb2
    for i in range(8):
        vel = 100 if i % 2 == 0 else 85  # accent downbeats
        bass_note(t + i * EIGHTH, Bb2, dur, vel)

    # Bar 2: Bb2 eighths for 6, then walk up D3-Eb3-F3-Bb2
    t2 = bar_start + 4 * BEAT
    for i in range(5):
        vel = 100 if i % 2 == 0 else 85
        bass_note(t2 + i * EIGHTH, Bb2, dur, vel)
    # Walk-up fill
    bass_note(t2 + 5 * EIGHTH, D3, EIGHTH * 0.7, 90)
    bass_note(t2 + 6 * EIGHTH, Eb3, EIGHTH * 0.7, 95)
    bass_note(t2 + 7 * EIGHTH, F3, EIGHTH * 0.7, 100)

# 8 repetitions of the 2-bar pattern = 16 bars
for rep in range(8):
    bass_pattern(rep * 8 * BEAT)


# ── Lead melody (starts bar 5) ────────────────────────────────────────
Bb4 = note('Bb4')  # 70
C5  = note('C5')   # 72
D5  = note('D5')   # 74
Eb5 = note('Eb5')  # 75
F5  = note('F5')   # 77
G4  = note('G4')   # 67
F4  = note('F4')   # 65
D4  = note('D4')   # 62

def lead_phrase_a(bar_start):
    """4-bar melodic phrase: ascending Bb pentatonic."""
    t = bar_start
    # Bar 1: Bb4 (dotted quarter) - D5 (eighth)
    note_on(t, Bb4, 80)
    note_off(t + BEAT * 1.5, Bb4)
    note_on(t + BEAT * 1.5, D5, 85)
    note_off(t + BEAT * 2, D5)
    # Rest, then F5 on beat 3
    note_on(t + BEAT * 2.5, F5, 90)
    note_off(t + BEAT * 3.5, F5)

    # Bar 2: Eb5 - D5 - Bb4 (descending)
    t += 4 * BEAT
    note_on(t, Eb5, 85)
    note_off(t + BEAT, Eb5)
    note_on(t + BEAT, D5, 80)
    note_off(t + BEAT * 2, D5)
    note_on(t + BEAT * 2, Bb4, 75)
    note_off(t + BEAT * 3.5, Bb4)

    # Bar 3: G4 - Bb4 (call)
    t += 4 * BEAT
    note_on(t + BEAT * 0.5, G4, 80)
    note_off(t + BEAT * 1.5, G4)
    note_on(t + BEAT * 2, Bb4, 85)
    note_off(t + BEAT * 3.5, Bb4)

    # Bar 4: F4 - D4 (response, descending)
    t += 4 * BEAT
    note_on(t, F4, 80)
    note_off(t + BEAT * 1.5, F4)
    note_on(t + BEAT * 2, D4, 75)
    note_off(t + BEAT * 3.5, D4)

def lead_phrase_b(bar_start):
    """4-bar variation: higher energy."""
    t = bar_start
    # Bar 1: F5 - Eb5 syncopated
    note_on(t + EIGHTH, F5, 90)
    note_off(t + BEAT * 1.5, F5)
    note_on(t + BEAT * 2, Eb5, 85)
    note_off(t + BEAT * 3, Eb5)
    note_on(t + BEAT * 3, D5, 80)
    note_off(t + BEAT * 3.75, D5)

    # Bar 2: Bb4 held
    t += 4 * BEAT
    note_on(t, Bb4, 85)
    note_off(t + BEAT * 3, Bb4)

    # Bar 3: C5 - D5 - F5 ascending run
    t += 4 * BEAT
    note_on(t + BEAT, C5, 80)
    note_off(t + BEAT * 1.5, C5)
    note_on(t + BEAT * 1.5, D5, 85)
    note_off(t + BEAT * 2, D5)
    note_on(t + BEAT * 2, F5, 90)
    note_off(t + BEAT * 3.5, F5)

    # Bar 4: D5 - Bb4 resolution
    t += 4 * BEAT
    note_on(t, D5, 80)
    note_off(t + BEAT * 2, D5)
    note_on(t + BEAT * 2.5, Bb4, 75)
    note_off(t + BEAT * 3.5, Bb4)

# Lead starts at bar 5 (bar_start = 4 * 4 * BEAT = 8000ms)
lead_phrase_a(4 * 4 * BEAT)     # bars 5-8
lead_phrase_b(8 * 4 * BEAT)     # bars 9-12
lead_phrase_a(12 * 4 * BEAT)    # bars 13-16


# ── Brass stabs (starts bar 9) ────────────────────────────────────────
Bb3 = note('Bb3')  # 58
D4b = note('D4')   # 62
F4b = note('F4')   # 65

def brass_stab(t, dur=EIGHTH * 1.5, vel=70):
    """Two-note brass chord: Bb3 + D4."""
    note_on(t, Bb3, vel)
    note_on(t, D4b, vel)
    note_off(t + dur, Bb3)
    note_off(t + dur, D4b)

def brass_pattern(bar_start):
    """2-bar brass pattern: stabs on beats 2 and 4."""
    for bar in range(2):
        t = bar_start + bar * 4 * BEAT
        brass_stab(t + BEAT, vel=70)          # beat 2
        brass_stab(t + 3 * BEAT, vel=65)      # beat 4

# Brass: bars 9-16
for rep in range(4):
    brass_pattern((8 + rep * 2) * 4 * BEAT)


# ── Sort and convert to delta-ms format ───────────────────────────────
events.sort(key=lambda e: e[0])

# Apply 4-voice polyphony limit
active_notes = []
limited = []
for abs_ms, etype, n, vel in events:
    if etype == "NoteOn":
        if len(active_notes) >= 4:
            stolen = active_notes.pop(0)
            limited.append((abs_ms, "NoteOff", stolen, 0))
        active_notes.append(n)
        limited.append((abs_ms, etype, n, vel))
    elif etype == "NoteOff":
        if n in active_notes:
            active_notes.remove(n)
        limited.append((abs_ms, etype, n, vel))

limited.sort(key=lambda e: e[0])

# Convert to delta-ms
delta_events = []
prev_ms = 0.0
for abs_ms, etype, n, vel in limited:
    delta = max(0, round(abs_ms - prev_ms))
    prev_ms = abs_ms
    delta_events.append((delta, etype, n, vel))

# ── Output C++ header ─────────────────────────────────────────────────
name = "dont_stop"

print("#pragma once")
print("// Auto-generated by generate_test_song.py — do not edit")
print("// Funky bass+lead+brass arrangement, Bb major, 120 BPM, 16 bars")
print("")
print('#include "../song_event.h"')
print("")
print("namespace pico_song {")
print("")
print(f"inline constexpr Event kEvents_{name}[] = {{")

for delta, etype, n, vel in delta_events:
    print(f"    {{{delta}, EventType::{etype}, {n}, {vel}}},")

print(f"    {{0, EventType::End, 0, 0}},")
print("};")
print("")

event_count = len(delta_events) + 1
print(f'inline constexpr Song kSong_{name} = {{"{name}", {BPM:.1f}f, '
      f"kEvents_{name}, {event_count}}};")
print("")
print("} // namespace pico_song")
print("")

print(f"// {len(delta_events)} note events + End sentinel = {event_count} total",
      file=sys.stderr)
print(f"// ~{event_count * 5} bytes flash", file=sys.stderr)
