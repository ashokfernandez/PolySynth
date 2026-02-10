import React from 'react';

const NOTE_ON = 20;
const NOTE_OFF = 21;

// Key layout for one octave (C4 to C5)
// White Key Width: 40px + 2px margin = 42px stride
// Black Key Width: 24px (centered on boundary)
// Boundary Offsets: 42, 84, 126, 168, 210, 252, 294
const KEYS = [
    { note: 60, type: 'white', label: 'C' },
    { note: 61, type: 'black', left: 30 }, // 42 - 12
    { note: 62, type: 'white', label: 'D' },
    { note: 63, type: 'black', left: 72 }, // 84 - 12
    { note: 64, type: 'white', label: 'E' },
    { note: 65, type: 'white', label: 'F' },
    { note: 66, type: 'black', left: 156 }, // 168 - 12
    { note: 67, type: 'white', label: 'G' },
    { note: 68, type: 'black', left: 198 }, // 210 - 12
    { note: 69, type: 'white', label: 'A' },
    { note: 70, type: 'black', left: 240 }, // 252 - 12
    { note: 71, type: 'white', label: 'B' },
    { note: 72, type: 'white', label: 'C' },
];

const Keyboard = ({ sendMsg }) => {
    const handleNoteOn = (note) => {
        sendMsg(NOTE_ON, note);
    };

    const handleNoteOff = (note) => {
        sendMsg(NOTE_OFF, note);
    };

    return (
        <div className="keyboard-panel">
            <div className="keyboard">
                {KEYS.map((key) => (
                    <div
                        key={key.note}
                        className={`key ${key.type}`}
                        style={key.type === 'black' ? { left: `${key.left}px` } : {}}
                        onMouseDown={() => handleNoteOn(key.note)}
                        onMouseUp={() => handleNoteOff(key.note)}
                        onMouseLeave={() => handleNoteOff(key.note)}
                        onTouchStart={(e) => { e.preventDefault(); handleNoteOn(key.note); }}
                        onTouchEnd={(e) => { e.preventDefault(); handleNoteOff(key.note); }}
                    />
                ))}
            </div>
        </div>
    );
};

export default Keyboard;
