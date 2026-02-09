import { render, screen, fireEvent } from '@testing-library/react';
import { describe, it, expect, vi } from 'vitest';
import Knob from '../Knob';

describe('Knob', () => {
    it('renders with label and value', () => {
        render(
            <Knob
                paramId={1}
                value={0.5}
                onChange={() => { }}
                label="TestKnob"
                min={0}
                max={100}
                unit="%"
                decimals={0}
            />
        );
        expect(screen.getByText('TestKnob')).toBeInTheDocument();
        expect(screen.getByText('50 %')).toBeInTheDocument();
    });

    it('considers mapValue prop for display', () => {
        render(
            <Knob
                paramId={1}
                value={0}
                onChange={() => { }}
                label="Shape"
                mapValue={(v) => v === 0 ? 'Sine' : 'Other'}
            />
        );
        expect(screen.getByText('Sine')).toBeInTheDocument();
    });

    // Interaction testing for drag is tricky with JSDOM but we can verify events are attached
    // or simulate mouse moves if we mock getBoundingClientRect or similar? 
    // For now, basic rendering is a good start. 
    // To test interaction we'd need to simulate mousedown, mousemove on window, mouseup.
});
