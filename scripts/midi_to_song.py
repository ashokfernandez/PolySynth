#!/usr/bin/env python3
"""Convert a MIDI file to a PolySynth Pico song header.

Zero external dependencies — raw MIDI parsing using struct.

Usage:
    python3 scripts/midi_to_song.py input.mid \
        --channels 1,2,3 \
        --start 18 --end 78 \
        --max-voices 4 \
        --name "dont_stop" \
        --output src/platform/pico/songs/dont_stop.h
"""

import argparse
import struct
import sys
from dataclasses import dataclass


@dataclass
class MidiEvent:
    abs_ms: float
    channel: int
    type: str  # "note_on" or "note_off"
    note: int
    velocity: int


def read_bytes(f, n):
    data = f.read(n)
    if len(data) < n:
        raise EOFError(f"Expected {n} bytes, got {len(data)}")
    return data


def read_variable_length(f):
    """Read a MIDI variable-length quantity."""
    value = 0
    while True:
        byte = struct.unpack("B", read_bytes(f, 1))[0]
        value = (value << 7) | (byte & 0x7F)
        if not (byte & 0x80):
            break
    return value


def parse_midi(filepath, channels, start_s, end_s):
    """Parse a Format 0 MIDI file and extract note events."""
    events = []

    with open(filepath, "rb") as f:
        # Read MThd header
        chunk_id = read_bytes(f, 4)
        if chunk_id != b"MThd":
            raise ValueError(f"Not a MIDI file: expected MThd, got {chunk_id!r}")

        header_len = struct.unpack(">I", read_bytes(f, 4))[0]
        fmt, num_tracks, division = struct.unpack(">HHH", read_bytes(f, 6))
        if header_len > 6:
            read_bytes(f, header_len - 6)  # skip extra header bytes

        if division & 0x8000:
            raise ValueError("SMPTE time division not supported, only ticks-per-beat")

        ticks_per_beat = division
        print(f"MIDI format {fmt}, {num_tracks} track(s), {ticks_per_beat} ticks/beat",
              file=sys.stderr)

        # Default tempo: 120 BPM
        us_per_beat = 500000  # 120 BPM
        detected_bpm = 120.0

        # Parse all tracks
        for track_idx in range(num_tracks):
            chunk_id = read_bytes(f, 4)
            if chunk_id != b"MTrk":
                raise ValueError(f"Expected MTrk, got {chunk_id!r}")
            track_len = struct.unpack(">I", read_bytes(f, 4))[0]
            track_end = f.tell() + track_len

            abs_ticks = 0
            running_status = 0

            while f.tell() < track_end:
                delta = read_variable_length(f)
                abs_ticks += delta

                # Calculate absolute time in ms
                abs_ms = (abs_ticks / ticks_per_beat) * (us_per_beat / 1000.0)

                # Peek at status byte
                peek = struct.unpack("B", read_bytes(f, 1))[0]

                if peek & 0x80:
                    status = peek
                    running_status = status
                else:
                    # Running status — use previous status byte
                    status = running_status
                    # peek is actually the first data byte
                    f.seek(-1, 1)

                msg_type = status & 0xF0
                channel = (status & 0x0F) + 1  # 1-based

                if msg_type == 0x80:  # Note Off
                    note = struct.unpack("B", read_bytes(f, 1))[0]
                    velocity = struct.unpack("B", read_bytes(f, 1))[0]
                    if channel in channels:
                        events.append(MidiEvent(abs_ms, channel, "note_off", note, 0))

                elif msg_type == 0x90:  # Note On
                    note = struct.unpack("B", read_bytes(f, 1))[0]
                    velocity = struct.unpack("B", read_bytes(f, 1))[0]
                    if channel in channels:
                        if velocity == 0:
                            events.append(MidiEvent(abs_ms, channel, "note_off", note, 0))
                        else:
                            events.append(MidiEvent(abs_ms, channel, "note_on", note, velocity))

                elif msg_type == 0xA0:  # Polyphonic aftertouch
                    read_bytes(f, 2)
                elif msg_type == 0xB0:  # Control change
                    read_bytes(f, 2)
                elif msg_type == 0xC0:  # Program change
                    read_bytes(f, 1)
                elif msg_type == 0xD0:  # Channel aftertouch
                    read_bytes(f, 1)
                elif msg_type == 0xE0:  # Pitch bend
                    read_bytes(f, 2)
                elif status == 0xFF:  # Meta event
                    meta_type = struct.unpack("B", read_bytes(f, 1))[0]
                    meta_len = read_variable_length(f)
                    meta_data = read_bytes(f, meta_len)

                    if meta_type == 0x51:  # Tempo
                        us_per_beat = (meta_data[0] << 16) | (meta_data[1] << 8) | meta_data[2]
                        detected_bpm = 60000000.0 / us_per_beat
                        print(f"  Tempo change: {detected_bpm:.1f} BPM at tick {abs_ticks}",
                              file=sys.stderr)

                elif status == 0xF0 or status == 0xF7:  # SysEx
                    sysex_len = read_variable_length(f)
                    read_bytes(f, sysex_len)
                else:
                    # Unknown status — try to skip
                    if msg_type in (0x80, 0x90, 0xA0, 0xB0, 0xE0):
                        read_bytes(f, 2)
                    elif msg_type in (0xC0, 0xD0):
                        read_bytes(f, 1)

            # Ensure we're at the end of the track
            f.seek(track_end)

    # Convert start/end to ms and filter
    start_ms = start_s * 1000.0
    end_ms = end_s * 1000.0

    filtered = [e for e in events if start_ms <= e.abs_ms <= end_ms]

    # Rebase times to start at 0
    if filtered:
        offset = filtered[0].abs_ms
        for e in filtered:
            e.abs_ms -= offset

    # Sort by time (stable — preserves order for simultaneous events)
    filtered.sort(key=lambda e: e.abs_ms)

    print(f"  {len(events)} total events, {len(filtered)} after trim [{start_s}s-{end_s}s]",
          file=sys.stderr)

    return filtered, detected_bpm


def apply_polyphony_limit(events, max_voices):
    """If >max_voices notes are active on a NoteOn, steal the oldest."""
    active = []  # list of (note, start_ms)
    output = []

    for ev in events:
        if ev.type == "note_on":
            if len(active) >= max_voices:
                # Steal oldest
                stolen = active.pop(0)
                output.append(MidiEvent(ev.abs_ms, ev.channel, "note_off", stolen[0], 0))
            active.append((ev.note, ev.abs_ms))
            output.append(ev)
        elif ev.type == "note_off":
            # Remove from active
            active = [(n, t) for n, t in active if n != ev.note]
            output.append(ev)
        else:
            output.append(ev)

    return output


def events_to_cpp(events, name, bpm):
    """Convert events to C++ constexpr header content."""
    lines = []
    lines.append("#pragma once")
    lines.append("// Auto-generated by midi_to_song.py — do not edit")
    lines.append("")
    lines.append('#include "../song_event.h"')
    lines.append("")
    lines.append("namespace pico_song {")
    lines.append("")
    lines.append(f"inline constexpr Event kEvents_{name}[] = {{")

    prev_ms = 0.0
    for ev in events:
        delta = max(0, round(ev.abs_ms - prev_ms))
        prev_ms = ev.abs_ms

        if ev.type == "note_on":
            lines.append(f"    {{{delta}, EventType::NoteOn, {ev.note}, {ev.velocity}}},")
        elif ev.type == "note_off":
            lines.append(f"    {{{delta}, EventType::NoteOff, {ev.note}, 0}},")

    # End sentinel
    lines.append(f"    {{0, EventType::End, 0, 0}},")
    lines.append("};")
    lines.append("")

    event_count = len(events) + 1  # +1 for End sentinel
    lines.append(f'inline constexpr Song kSong_{name} = {{"{name}", {bpm:.1f}f, '
                 f"kEvents_{name}, {event_count}}};")
    lines.append("")
    lines.append("} // namespace pico_song")
    lines.append("")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Convert MIDI to PolySynth Pico song header")
    parser.add_argument("midi_file", help="Input MIDI file")
    parser.add_argument("--channels", default="1,2,3",
                        help="Comma-separated MIDI channels to include (1-based)")
    parser.add_argument("--start", type=float, default=0.0,
                        help="Start time in seconds")
    parser.add_argument("--end", type=float, default=999.0,
                        help="End time in seconds")
    parser.add_argument("--max-voices", type=int, default=4,
                        help="Maximum polyphony (default: 4)")
    parser.add_argument("--name", default="song",
                        help="C++ identifier name for the song")
    parser.add_argument("--output", default=None,
                        help="Output file path (default: stdout)")

    args = parser.parse_args()

    channels = set(int(c) for c in args.channels.split(","))
    events, bpm = parse_midi(args.midi_file, channels, args.start, args.end)
    events = apply_polyphony_limit(events, args.max_voices)

    cpp = events_to_cpp(events, args.name, bpm)

    total_bytes = len(events) * 5  # 5 bytes per Event struct
    print(f"  Output: {len(events)} events, ~{total_bytes} bytes flash", file=sys.stderr)

    if args.output:
        with open(args.output, "w") as f:
            f.write(cpp)
        print(f"  Written to: {args.output}", file=sys.stderr)
    else:
        print(cpp)


if __name__ == "__main__":
    main()
