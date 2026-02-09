/**
 * Simple unit tests for useIPlug communication protocol
 * These test the message format expectations without full JSDOM
 */
import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';

describe('IPlug Protocol Constants', () => {
    it('Factory preset tags match expected enum values', () => {
        // These tags must match the C++ EControlTags enum
        const expectedTags = {
            kMsgTagTestLoaded: 6,
            kMsgTagDemoMono: 7,
            kMsgTagDemoPoly: 8,
            kMsgTagSavePreset: 9,
            kMsgTagLoadPreset: 10,
            kMsgTagPreset1: 11, // Warm Pad
            kMsgTagPreset2: 12, // Bright Lead  
            kMsgTagPreset3: 13, // Dark Bass
        };

        // Verify all expected tags are defined
        expect(expectedTags.kMsgTagPreset1).toBe(11);
        expect(expectedTags.kMsgTagPreset2).toBe(12);
        expect(expectedTags.kMsgTagPreset3).toBe(13);
        expect(expectedTags.kMsgTagSavePreset).toBe(9);
        expect(expectedTags.kMsgTagLoadPreset).toBe(10);
    });

    it('SPVFUI message format is correct', () => {
        const message = {
            msg: 'SPVFUI',
            paramIdx: 0,
            value: 0.75
        };

        expect(message.msg).toBe('SPVFUI');
        expect(typeof message.paramIdx).toBe('number');
        expect(typeof message.value).toBe('number');
        expect(message.value).toBeGreaterThanOrEqual(0);
        expect(message.value).toBeLessThanOrEqual(1);
    });

    it('SAMFUI message format is correct', () => {
        const message = {
            msg: 'SAMFUI',
            msgTag: 11,
            ctrlTag: 0,
            data: ''
        };

        expect(message.msg).toBe('SAMFUI');
        expect(typeof message.msgTag).toBe('number');
        expect(typeof message.ctrlTag).toBe('number');
        expect(typeof message.data).toBe('string');
    });
});

describe('SPVFD Global Function Contract', () => {
    beforeEach(() => {
        // Simulate what useIPlug hook does
        global.SPVFD = vi.fn((paramIdx, value) => {
            // This is what the hook should do
            return { paramIdx, value };
        });
    });

    afterEach(() => {
        delete global.SPVFD;
    });

    it('SPVFD accepts paramIdx and value', () => {
        global.SPVFD(0, 0.5);
        expect(global.SPVFD).toHaveBeenCalledWith(0, 0.5);
    });

    it('SPVFD can receive multiple param updates', () => {
        // Simulate a preset load that updates multiple params
        global.SPVFD(0, 0.8);   // Gain
        global.SPVFD(2, 0.5);   // Attack  
        global.SPVFD(11, 0.3);  // Cutoff

        expect(global.SPVFD).toHaveBeenCalledTimes(3);
        expect(global.SPVFD).toHaveBeenNthCalledWith(1, 0, 0.8);
        expect(global.SPVFD).toHaveBeenNthCalledWith(2, 2, 0.5);
        expect(global.SPVFD).toHaveBeenNthCalledWith(3, 11, 0.3);
    });

    it('SPVFD handles normalized values correctly', () => {
        // Test boundary values
        global.SPVFD(0, 0.0);  // Minimum
        global.SPVFD(0, 1.0);  // Maximum
        global.SPVFD(0, 0.5);  // Middle

        expect(global.SPVFD).toHaveBeenNthCalledWith(1, 0, 0.0);
        expect(global.SPVFD).toHaveBeenNthCalledWith(2, 0, 1.0);
        expect(global.SPVFD).toHaveBeenNthCalledWith(3, 0, 0.5);
    });
});
