import React, { useState, useEffect, useRef } from 'react';
import PropTypes from 'prop-types';

const Knob = ({ paramId, value = 0, onChange, label, min = 0, max = 100, unit = '', decimals = 0, mapValue }) => {
    const [dragging, setDragging] = useState(false);
    const startY = useRef(0);
    const startValue = useRef(0);
    const knobRef = useRef(null);

    const handleMouseDown = (e) => {
        setDragging(true);
        startY.current = e.clientY;
        startValue.current = value;
        document.body.style.cursor = 'ns-resize';
    };

    useEffect(() => {
        const handleMouseMove = (e) => {
            if (!dragging) return;
            const delta = startY.current - e.clientY;
            const sensitivity = 0.005;
            let newValue = Math.min(1, Math.max(0, startValue.current + delta * sensitivity));
            onChange(paramId, newValue);
        };

        const handleMouseUp = () => {
            if (dragging) {
                setDragging(false);
                document.body.style.cursor = 'default';
            }
        };

        if (dragging) {
            window.addEventListener('mousemove', handleMouseMove);
            window.addEventListener('mouseup', handleMouseUp);
        }

        return () => {
            window.removeEventListener('mousemove', handleMouseMove);
            window.removeEventListener('mouseup', handleMouseUp);
        };
    }, [dragging, onChange, paramId]);

    // Visuals
    const minAngle = -135;
    const maxAngle = 135;
    const angle = minAngle + (value * (maxAngle - minAngle));

    // Display value formatting
    let displayValue = '';
    if (mapValue) {
        displayValue = mapValue(value);
    } else {
        const actualValue = min + (value * (max - min));
        displayValue = actualValue.toFixed(decimals) + (unit ? ` ${unit}` : '');
    }

    return (
        <div className="control">
            <div
                ref={knobRef}
                className="knob"
                style={{ transform: `rotate(${angle}deg)` }}
                onMouseDown={handleMouseDown}
            >
                <div className="knob-indicator"></div>
            </div>
            <label>{label}</label>
            <div className="param-value">{displayValue}</div>
        </div>
    );
};

Knob.propTypes = {
    paramId: PropTypes.number.isRequired,
    value: PropTypes.number,
    onChange: PropTypes.func.isRequired,
    label: PropTypes.string,
    min: PropTypes.number,
    max: PropTypes.number,
    unit: PropTypes.string,
    decimals: PropTypes.number,
    mapValue: PropTypes.func
};

export default Knob;
