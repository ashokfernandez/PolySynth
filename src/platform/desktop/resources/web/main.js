document.addEventListener('DOMContentLoaded', () => {
    console.log("PolySynth UI Loaded");

    const paramMeta = {
        0: { name: 'Gain', min: 0, max: 100, unit: '%', decimals: 0 },
        2: { name: 'Attack', min: 1, max: 1000, unit: 'ms', decimals: 0 },
        3: { name: 'Decay', min: 1, max: 1000, unit: 'ms', decimals: 0 },
        4: { name: 'Sustain', min: 0, max: 100, unit: '%', decimals: 0 },
        5: { name: 'Release', min: 2, max: 1000, unit: 'ms', decimals: 0 },
        6: { name: 'LFO Shape', items: ['Sine', 'Triangle', 'Square', 'Saw'] },
        7: { name: 'LFO Rate', min: 0.01, max: 40, unit: 'Hz', decimals: 2 },
        10: { name: 'LFO Depth', min: 0, max: 100, unit: '%', decimals: 0 },
        11: { name: 'Cutoff', min: 20, max: 20000, unit: 'Hz', decimals: 0 },
        12: { name: 'Resonance', min: 0, max: 100, unit: '%', decimals: 0 },
        13: { name: 'Osc Wave', items: ['Saw', 'Square', 'Triangle', 'Sine'] },
        14: { name: 'Osc Mix', min: 0, max: 100, unit: '%', decimals: 0 }
    };

    const controls = new Map();
    document.querySelectorAll('[data-param-idx]').forEach((el) => {
        const idx = Number(el.dataset.paramIdx);
        if (!Number.isNaN(idx)) {
            controls.set(idx, {
                input: el,
                display: el.parentElement.querySelector('[data-param-value]') || null
            });
        }
    });

    const clamp01 = (value) => Math.min(1, Math.max(0, value));

    const toDisplayValue = (paramIdx, normalized) => {
        const meta = paramMeta[paramIdx];
        if (!meta) {
            return `${(normalized * 100).toFixed(0)}%`;
        }

        if (meta.items) {
            const index = Math.min(meta.items.length - 1, Math.floor(normalized * meta.items.length));
            return `${meta.name}: ${meta.items[index]}`;
        }

        const value = meta.min + (meta.max - meta.min) * normalized;
        return `${meta.name}: ${value.toFixed(meta.decimals)}${meta.unit ? ` ${meta.unit}` : ''}`;
    };

    const updateValueDisplay = (paramIdx, normalized) => {
        const entry = controls.get(paramIdx);
        if (!entry || !entry.display) return;
        entry.display.textContent = toDisplayValue(paramIdx, normalized);
    };

    const updateKnobVisual = (knob, value) => {
        const minAngle = -135;
        const maxAngle = 135;
        const angle = minAngle + (value * (maxAngle - minAngle));
        knob.style.transform = `rotate(${angle}deg)`;
    };

    const knobState = new Map();
    const animateKnobTo = (paramIdx, targetValue) => {
        const entry = controls.get(paramIdx);
        if (!entry || !entry.input.classList.contains('knob')) return;

        const state = knobState.get(paramIdx) || { value: targetValue, target: targetValue, animating: false };
        state.target = targetValue;
        if (!state.animating) {
            state.animating = true;
            const step = () => {
                const diff = state.target - state.value;
                state.value += diff * 0.2;
                if (Math.abs(diff) < 0.001) {
                    state.value = state.target;
                    state.animating = false;
                } else {
                    requestAnimationFrame(step);
                }
                updateKnobVisual(entry.input, state.value);
                updateValueDisplay(paramIdx, state.value);
            };
            requestAnimationFrame(step);
        }
        knobState.set(paramIdx, state);
    };

    window.SPVFUI = {
        setParam: (paramIdx, normalizedValue) => {
            const value = clamp01(normalizedValue);
            if (window.IPlugSendParamValue) {
                window.IPlugSendParamValue(paramIdx, value);
                return;
            }
            if (window.IPlugSendMsg) {
                window.IPlugSendMsg({
                    msg: 'SPVFUI',
                    paramIdx,
                    value
                });
            } else {
                console.warn("IPlugSendParamValue/IPlugSendMsg not found (running in browser?)");
            }
        },
        paramChanged: (paramIdx, normalizedValue) => {
            const value = clamp01(normalizedValue);
            const entry = controls.get(paramIdx);
            if (!entry) return;

            if (entry.input.classList.contains('knob')) {
                animateKnobTo(paramIdx, value);
            } else {
                entry.input.value = value;
                updateValueDisplay(paramIdx, value);
                drawEnvelope();
            }
        },
        initParams: (paramArray) => {
            if (!Array.isArray(paramArray)) return;
            paramArray.forEach((value, idx) => {
                if (typeof value === 'number') {
                    window.SPVFUI.paramChanged(idx, value);
                }
            });
        }
    };

    const knobs = document.querySelectorAll('.knob[data-param-idx]');
    knobs.forEach(knob => {
        let isDragging = false;
        let startY = 0;
        let currentValue = 0.5;
        const paramIdx = Number(knob.dataset.paramIdx);

        knob.addEventListener('mousedown', (e) => {
            isDragging = true;
            startY = e.clientY;
            // Sync current value with state to prevent jumping
            const state = knobState.get(paramIdx);
            if (state) {
                currentValue = state.value;
            }
            document.body.style.cursor = 'ns-resize';
            e.preventDefault();
        });

        window.addEventListener('mousemove', (e) => {
            if (!isDragging) return;
            const delta = startY - e.clientY;
            currentValue = clamp01(currentValue + delta * 0.005);
            updateKnobVisual(knob, currentValue);
            updateValueDisplay(paramIdx, currentValue);
            window.SPVFUI.setParam(paramIdx, currentValue);
            startY = e.clientY;
        });

        window.addEventListener('mouseup', () => {
            isDragging = false;
            document.body.style.cursor = 'default';
        });

        updateKnobVisual(knob, currentValue);
        updateValueDisplay(paramIdx, currentValue);
        knobState.set(paramIdx, { value: currentValue, target: currentValue, animating: false });
    });

    const faders = document.querySelectorAll('.fader[data-param-idx]');
    faders.forEach(fader => {
        const paramIdx = Number(fader.dataset.paramIdx);
        fader.addEventListener('input', (e) => {
            const val = clamp01(parseFloat(e.target.value));
            updateValueDisplay(paramIdx, val);
            window.SPVFUI.setParam(paramIdx, val);
            drawEnvelope();
        });
        updateValueDisplay(paramIdx, clamp01(parseFloat(fader.value)));
    });

    const canvas = document.getElementById('adsr-canvas');
    let drawEnvelope = () => { };
    if (canvas) {
        const ctx = canvas.getContext('2d');
        const dpr = window.devicePixelRatio || 1;
        const rect = canvas.getBoundingClientRect();
        canvas.width = rect.width * dpr;
        canvas.height = rect.height * dpr;
        ctx.scale(dpr, dpr);

        drawEnvelope = () => {
            const w = rect.width;
            const h = rect.height;

            ctx.clearRect(0, 0, w, h);
            ctx.strokeStyle = '#007bff';
            ctx.lineWidth = 2;
            ctx.beginPath();

            const attack = parseFloat(document.querySelector('[data-param="AmpAttack"]').value) || 0.1;
            const decay = parseFloat(document.querySelector('[data-param="AmpDecay"]').value) || 0.1;
            const sustain = parseFloat(document.querySelector('[data-param="AmpSustain"]').value) || 0.5;
            const release = parseFloat(document.querySelector('[data-param="AmpRelease"]').value) || 0.2;

            const attackX = w * (attack * 0.25);
            const decayX = attackX + (w * (decay * 0.25));
            const releaseX = w - (w * (release * 0.25));

            const sustainLevel = h - (sustain * h);

            ctx.moveTo(0, h);
            ctx.lineTo(attackX, 0);
            ctx.lineTo(decayX, sustainLevel);
            ctx.lineTo(releaseX, sustainLevel);
            ctx.lineTo(w, h);

            ctx.stroke();

            ctx.fillStyle = 'rgba(0, 123, 255, 0.1)';
            ctx.lineTo(w, h);
            ctx.lineTo(0, h);
            ctx.fill();
        };

        drawEnvelope();
        window.drawEnvelope = drawEnvelope;
    }

    if (window.IPlugSendMsg) {
        // Send kMsgTagTestLoaded (6) to signal UI load
        window.IPlugSendMsg({
            msg: 'SAMFUI',
            msgTag: 6,
            ctrlTag: 0,
            data: ''
        });
    }
});
