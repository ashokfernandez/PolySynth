import React, { useRef, useEffect } from 'react';
import PropTypes from 'prop-types';

const Envelope = ({ attack, decay, sustain, release }) => {
    const canvasRef = useRef(null);
    const containerRef = useRef(null);

    useEffect(() => {
        const canvas = canvasRef.current;
        const container = containerRef.current;
        if (!canvas || !container) return;

        const ctx = canvas.getContext('2d');
        const dpr = window.devicePixelRatio || 1;

        // Resize handling
        const resize = () => {
            const rect = container.getBoundingClientRect();
            canvas.width = rect.width * dpr;
            canvas.height = rect.height * dpr;
            ctx.scale(dpr, dpr);
            draw(rect.width, rect.height);
        };

        const draw = (w, h) => {
            ctx.clearRect(0, 0, w, h);
            ctx.strokeStyle = '#007bff';
            ctx.lineWidth = 2;
            ctx.beginPath();

            // Simple visualization logic:
            // Total width is split into 4 sections roughly, or proportional to times?
            // The original code was:
            // attackX = w * (attack * 0.25)
            // decayX = attackX + (w * (decay * 0.25))
            // releaseX = w - (w * (release * 0.25))

            // Allow minimal values for visualization so it doesn't disappear
            const a = Math.max(0.01, attack);
            const d = Math.max(0.01, decay);
            const s = sustain;
            const r = Math.max(0.01, release);

            const attackX = w * (a * 0.25);
            const decayX = attackX + (w * (d * 0.25));
            // Release starts after some hold time? 
            // In the original, releaseX was calculated from the end.
            const releaseX = w - (w * (r * 0.25));
            // Let's ensure release starts after decay
            const sustainEndX = Math.max(decayX, releaseX - (w * 0.1)); // Ensure some sustain line

            const sustainLevel = h - (s * h);

            ctx.moveTo(0, h);
            ctx.lineTo(attackX, 0);
            ctx.lineTo(decayX, sustainLevel);
            ctx.lineTo(releaseX, sustainLevel);
            ctx.lineTo(w, h);

            ctx.stroke();

            ctx.fillStyle = 'rgba(0, 123, 255, 0.1)';
            ctx.lineTo(w, h); // ensure closed path at bottom right
            ctx.lineTo(0, h);
            ctx.fill();
        };

        resize();
        window.addEventListener('resize', resize);
        return () => window.removeEventListener('resize', resize);
    }, [attack, decay, sustain, release]);

    return (
        <div className="envelope-visualizer-container" ref={containerRef}>
            <canvas ref={canvasRef} />
        </div>
    );
};

Envelope.propTypes = {
    attack: PropTypes.number.isRequired,
    decay: PropTypes.number.isRequired,
    sustain: PropTypes.number.isRequired,
    release: PropTypes.number.isRequired
};

export default Envelope;
