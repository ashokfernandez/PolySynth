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

    it('sends message when demo button is clicked', () => {
        render(<App />);
        const monoBtn = screen.getByText('Demo Mono');
        fireEvent.click(monoBtn);
        // Expect active class to be present? 
        // Note: checking class requires inspecting the element.
        expect(monoBtn.className).toContain('active');
        expect(window.IPlugSendMsg).toHaveBeenLastCalledWith(expect.objectContaining({
            msg: 'SAMFUI',
            msgTag: 7
        }));
    });
});
