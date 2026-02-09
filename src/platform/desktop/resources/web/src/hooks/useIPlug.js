import { useState, useEffect, useCallback } from 'react';

// Mock for development in browser
if (!window.IPlugSendMsg) {
    window.IPlugSendMsg = (msg) => {
        console.log('IPlugSendMsg:', msg);
    };
}

export const useIPlug = () => {
    const [params, setParams] = useState(new Map());

    const setParam = useCallback((paramIdx, normalizedValue) => {
        // Optimistic update
        setParams(prev => {
            const next = new Map(prev);
            next.set(paramIdx, normalizedValue);
            return next;
        });

        // Send to C++
        if (window.IPlugSendParamValue) {
            window.IPlugSendParamValue(paramIdx, normalizedValue);
        } else if (window.IPlugSendMsg) {
            window.IPlugSendMsg({
                msg: 'SPVFUI',
                paramIdx,
                value: normalizedValue
            });
        }
    }, []);

    const sendMsg = useCallback((msgTag, ctrlTag = 0, data = '') => {
        if (window.IPlugSendMsg) {
            window.IPlugSendMsg({
                msg: 'SAMFUI',
                msgTag,
                ctrlTag,
                data
            });
        }
    }, []);

    useEffect(() => {
        // Expose global SPVFD function for C++ WebViewEditorDelegate to call
        // The C++ side calls: SPVFD(paramIdx, value)
        window.SPVFD = (paramIdx, value) => {
            console.log('SPVFD received:', paramIdx, value);
            setParams(prev => {
                const next = new Map(prev);
                next.set(paramIdx, value);
                return next;
            });
        };

        // Also expose legacy names for compatibility
        window.SPVFUI = {
            paramChanged: (paramIdx, value) => {
                window.SPVFD(paramIdx, value);
            },
            initParams: (paramArray) => {
                if (Array.isArray(paramArray)) {
                    setParams(prev => {
                        const next = new Map(prev);
                        paramArray.forEach((val, idx) => {
                            if (typeof val === 'number') {
                                next.set(idx, val);
                            }
                        });
                        return next;
                    });
                }
            }
        };

        // Notify C++ we are ready
        sendMsg(6); // kMsgTagTestLoaded

        return () => {
            delete window.SPVFD;
            delete window.SPVFUI;
        };
    }, [sendMsg]);

    return { params, setParam, sendMsg };
};
