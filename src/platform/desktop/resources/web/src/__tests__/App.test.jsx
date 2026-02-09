import { render, screen, fireEvent } from '@testing-library/react';
import { describe, it, expect, vi } from 'vitest';
import App from '../App';

// Mock IPlug global
window.IPlugSendMsg = vi.fn();
window.IPlugSendParamValue = vi.fn();

// Mock Envelope to avoid canvas issues in test environment
vi.mock('../components/Envelope', () => ({
    default: () => <div data-testid="envelope-mock">Envelope</div>
}));

describe('App Integration', () => {
    it('renders the main title', () => {
        render(<App />);
        expect(screen.getByText('PolySynth')).toBeInTheDocument();
    });

    it('renders sections', () => {
        render(<App />);
        expect(screen.getByText('Oscillators')).toBeInTheDocument();
        expect(screen.getByText('Filter')).toBeInTheDocument();
        expect(screen.getByText('LFO')).toBeInTheDocument();
        expect(screen.getByTestId('envelope-mock')).toBeInTheDocument();
    });

    it('renders Presets section', () => {
        render(<App />);
        expect(screen.getByText('Presets')).toBeInTheDocument();
        expect(screen.getByText('Factory Presets')).toBeInTheDocument();
        expect(screen.getByText('User Preset')).toBeInTheDocument();
    });

    it('renders factory preset buttons', () => {
        render(<App />);
        expect(screen.getByText('🎹 Warm Pad')).toBeInTheDocument();
        expect(screen.getByText('⚡ Bright Lead')).toBeInTheDocument();
        expect(screen.getByText('🎸 Dark Bass')).toBeInTheDocument();
    });

    it('renders user preset buttons', () => {
        render(<App />);
        expect(screen.getByText('💾 Save')).toBeInTheDocument();
        expect(screen.getByText('📂 Load')).toBeInTheDocument();
    });

    it('sends message when demo button is clicked', () => {
        render(<App />);
        const monoBtn = screen.getByText('🎵 Mono');
        fireEvent.click(monoBtn);
        expect(monoBtn.className).toContain('active');
        expect(window.IPlugSendMsg).toHaveBeenLastCalledWith(expect.objectContaining({
            msg: 'SAMFUI',
            msgTag: 7
        }));
    });

    it('sends preset message when factory preset is clicked', () => {
        render(<App />);
        const warmPadBtn = screen.getByText('🎹 Warm Pad');
        fireEvent.click(warmPadBtn);
        expect(warmPadBtn.className).toContain('active');
        expect(window.IPlugSendMsg).toHaveBeenLastCalledWith(expect.objectContaining({
            msg: 'SAMFUI',
            msgTag: 11  // kMsgTagPreset1
        }));
    });

    it('sends save message when Save button is clicked', () => {
        render(<App />);
        const saveBtn = screen.getByText('💾 Save');
        fireEvent.click(saveBtn);
        expect(window.IPlugSendMsg).toHaveBeenLastCalledWith(expect.objectContaining({
            msg: 'SAMFUI',
            msgTag: 9  // kMsgTagSavePreset
        }));
    });
});
