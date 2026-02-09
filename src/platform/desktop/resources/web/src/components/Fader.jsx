import React from 'react';
import PropTypes from 'prop-types';

const Fader = ({ paramId, value = 0, onChange, label, min = 0, max = 1, step = 0.01 }) => {
    return (
        <div className="fader-control">
            <input
                type="range"
                className="fader"
                min={0}
                max={1}
                step={step}
                value={value}
                onChange={(e) => onChange(paramId, parseFloat(e.target.value))}
            />
            <label>{label}</label>
            <div className="param-value">{value.toFixed(2)}</div>
        </div>
    );
};

Fader.propTypes = {
    paramId: PropTypes.number.isRequired,
    value: PropTypes.number,
    onChange: PropTypes.func.isRequired,
    label: PropTypes.string,
    min: PropTypes.number,
    max: PropTypes.number,
    step: PropTypes.number
};

export default Fader;
