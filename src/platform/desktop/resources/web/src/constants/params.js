export const PARAMS = {
    GAIN: 0,
    ATTACK: 2,
    DECAY: 3,
    SUSTAIN: 4,
    RELEASE: 5,
    LFO_SHAPE: 6,
    LFO_RATE: 7,
    LFO_DEPTH: 10,
    CUTOFF: 11,
    RESONANCE: 12,
    OSC_WAVE: 13,
    OSC_MIX: 14,
    CHORUS_RATE: 15,
    CHORUS_DEPTH: 16,
    CHORUS_MIX: 17,
    DELAY_TIME: 18,
    DELAY_FEEDBACK: 19,
    DELAY_MIX: 20,
    LIMITER_THRESHOLD: 21
};

export const PARAM_META = {
    [PARAMS.GAIN]: { name: 'Gain', min: 0, max: 100, unit: '%', decimals: 0 },
    [PARAMS.ATTACK]: { name: 'Attack', min: 1, max: 1000, unit: 'ms', decimals: 0 },
    [PARAMS.DECAY]: { name: 'Decay', min: 1, max: 1000, unit: 'ms', decimals: 0 },
    [PARAMS.SUSTAIN]: { name: 'Sustain', min: 0, max: 100, unit: '%', decimals: 0 },
    [PARAMS.RELEASE]: { name: 'Release', min: 2, max: 1000, unit: 'ms', decimals: 0 },
    [PARAMS.LFO_SHAPE]: { name: 'LFO Shape', items: ['Sine', 'Triangle', 'Square', 'Saw'] },
    [PARAMS.LFO_RATE]: { name: 'LFO Rate', min: 0.01, max: 40, unit: 'Hz', decimals: 2 },
    [PARAMS.LFO_DEPTH]: { name: 'LFO Depth', min: 0, max: 100, unit: '%', decimals: 0 },
    [PARAMS.CUTOFF]: { name: 'Cutoff', min: 20, max: 20000, unit: 'Hz', decimals: 0 },
    [PARAMS.RESONANCE]: { name: 'Resonance', min: 0, max: 100, unit: '%', decimals: 0 },
    [PARAMS.OSC_WAVE]: { name: 'Osc Wave', items: ['Saw', 'Square', 'Triangle', 'Sine'] },
    [PARAMS.OSC_MIX]: { name: 'Osc Mix', min: 0, max: 100, unit: '%', decimals: 0 },
    [PARAMS.CHORUS_RATE]: { name: 'Chorus Rate', min: 0.05, max: 2, unit: 'Hz', decimals: 2 },
    [PARAMS.CHORUS_DEPTH]: { name: 'Chorus Depth', min: 0, max: 100, unit: '%', decimals: 0 },
    [PARAMS.CHORUS_MIX]: { name: 'Chorus Mix', min: 0, max: 100, unit: '%', decimals: 0 },
    [PARAMS.DELAY_TIME]: { name: 'Delay Time', min: 50, max: 1200, unit: 'ms', decimals: 0 },
    [PARAMS.DELAY_FEEDBACK]: { name: 'Delay Feedback', min: 0, max: 95, unit: '%', decimals: 0 },
    [PARAMS.DELAY_MIX]: { name: 'Delay Mix', min: 0, max: 100, unit: '%', decimals: 0 },
    [PARAMS.LIMITER_THRESHOLD]: { name: 'Limiter Threshold', min: 50, max: 100, unit: '%', decimals: 0 }
};

export const mapParamValue = (idx, normalized) => {
    const meta = PARAM_META[idx];
    if (!meta) return '';

    if (meta.items) {
        const index = Math.min(meta.items.length - 1, Math.floor(normalized * meta.items.length));
        return meta.items[index];
    }
    return ''; // Default formatting logic handles range mapping
};
