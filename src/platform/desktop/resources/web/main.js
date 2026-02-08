document.addEventListener('DOMContentLoaded', () => {
    console.log("PolySynth UI Loaded");

    // iPlug2 Bridge Setup
    // Recieve from C++
    window.SPVFD = (paramIdx, val) => {
        console.log(`[Bridge] Recv Parameter: ${paramIdx} = ${val}`);

        // Find controls bound to this paramIdx and update them
        // Note: In real app, might need a map of paramIdx to DOM elements
        // For now, simple selector query
        // We need to map param IDs (strings in HTML) to indices (integers)
        // This requires a mapping system. 
        // For this prototype, we assume data-param is the INDEX (or we map it)

        // For the prototype, we will just log it unless we map strings to ints.
        // But the UI sends strings? No, HTML has data-param="Osc1Wave".

        // Strategy: We need a map.
        // For now, update the visual if we can find it.
        // This part is tricky without shared constants.
    }

    window.PolySynthBridge = {
        sendParameter: (paramId, value) => {
            console.log(`[Bridge] Send Parameter: ${paramId} = ${value}`);
            // Map string IDs to indices (Hardcoded for prototype Match)
            const paramMap = {
                "MasterGain": 0,
                "NoteGlideTime": 1,
                "AmpAttack": 2,
                "AmpDecay": 3,
                "AmpSustain": 4,
                "AmpRelease": 5,
                "LFO Shape": 6, // Matches C++ roughly
                "LFO Rate": 7,
                "OscMix": -1, // Not in C++ params list
                "Osc1Wave": -1,
                "FilterCutoff": -1,
                "FilterRes": -1
            };

            const idx = paramMap[paramId];
            if (idx !== undefined && idx >= 0) {
                var message = {
                    "msg": "SPVFUI",
                    "paramIdx": idx,
                    "value": value
                };
                if (window.IPlugSendMsg) {
                    window.IPlugSendMsg(message);
                } else {
                    console.warn("IPlugSendMsg not found (running in browser?)");
                }
            }
        }
    };

    // Interactive Knobs (Simple Vertical Drag)
    const knobs = document.querySelectorAll('.knob');

    knobs.forEach(knob => {
        let isDragging = false;
        let startY = 0;
        let currentValue = 0.5; // Normalized 0-1

        knob.addEventListener('mousedown', (e) => {
            isDragging = true;
            startY = e.clientY;
            document.body.style.cursor = 'ns-resize';
            e.preventDefault();
        });

        window.addEventListener('mousemove', (e) => {
            if (!isDragging) return;

            const delta = startY - e.clientY;
            currentValue += delta * 0.005; // Sensitivity
            currentValue = Math.min(Math.max(currentValue, 0), 1);

            updateKnobVisual(knob, currentValue);

            const paramId = knob.parentElement.dataset.param || "Unknown";
            window.PolySynthBridge.sendParameter(paramId, currentValue);

            startY = e.clientY; // Reset for relative movement
        });

        window.addEventListener('mouseup', () => {
            isDragging = false;
            document.body.style.cursor = 'default';
        });

        // Initialize
        updateKnobVisual(knob, currentValue);
    });

    function updateKnobVisual(knob, value) {
        // Rotate from -135deg to +135deg (270 degree range)
        const minAngle = -135;
        const maxAngle = 135;
        const angle = minAngle + (value * (maxAngle - minAngle));

        knob.style.transform = `rotate(${angle}deg)`;
    }

    // Fader Handling
    const faders = document.querySelectorAll('.fader');
    faders.forEach(fader => {
        fader.addEventListener('input', (e) => {
            const val = parseFloat(e.target.value);
            const paramId = e.target.dataset.param;
            window.PolySynthBridge.sendParameter(paramId, val);
            drawEnvelope(); // Re-draw ADSR when changed
        });
    });

    // Canvas ADSR Drawing
    const canvas = document.getElementById('adsr-canvas');
    if (canvas) {
        const ctx = canvas.getContext('2d');

        // Handle HiDPI displays
        const dpr = window.devicePixelRatio || 1;
        const rect = canvas.getBoundingClientRect();
        canvas.width = rect.width * dpr;
        canvas.height = rect.height * dpr;
        ctx.scale(dpr, dpr);

        function drawEnvelope() {
            const w = rect.width;
            const h = rect.height;

            ctx.clearRect(0, 0, w, h);
            ctx.strokeStyle = '#007bff';
            ctx.lineWidth = 2;
            ctx.beginPath();

            // Fetch values from faders
            const attack = parseFloat(document.querySelector('[data-param="AmpAttack"]').value) || 0.1;
            const decay = parseFloat(document.querySelector('[data-param="AmpDecay"]').value) || 0.1;
            const sustain = parseFloat(document.querySelector('[data-param="AmpSustain"]').value) || 0.5;
            const release = parseFloat(document.querySelector('[data-param="AmpRelease"]').value) || 0.2;

            // Simple visualization logic
            // Total width is fixed, distribute segments roughly
            const attackX = w * (attack * 0.25);
            const decayX = attackX + (w * (decay * 0.25));
            const releaseX = w - (w * (release * 0.25));

            const sustainLevel = h - (sustain * h);

            ctx.moveTo(0, h); // Start bottom left
            ctx.lineTo(attackX, 0); // Peak
            ctx.lineTo(decayX, sustainLevel); // Sustain Level
            ctx.lineTo(releaseX, sustainLevel); // Sustain Hold
            ctx.lineTo(w, h); // End

            ctx.stroke();

            // Fill
            ctx.fillStyle = 'rgba(0, 123, 255, 0.1)';
            ctx.lineTo(w, h);
            ctx.lineTo(0, h);
            ctx.fill();
        }

        // Initial draw
        drawEnvelope();
        window.drawEnvelope = drawEnvelope; // Expose for debug
    }
});
