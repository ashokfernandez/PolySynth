import { describe, it, expect } from 'vitest';
import { PARAMS, PARAM_META } from '../constants/params';

describe('FX parameter metadata', () => {
  it('registers chorus parameters', () => {
    expect(PARAMS.CHORUS_RATE).toBeTypeOf('number');
    expect(PARAM_META[PARAMS.CHORUS_RATE].unit).toBe('Hz');
    expect(PARAM_META[PARAMS.CHORUS_DEPTH].max).toBe(100);
    expect(PARAM_META[PARAMS.CHORUS_MIX].min).toBe(0);
  });

  it('registers delay parameters', () => {
    expect(PARAMS.DELAY_TIME).toBeTypeOf('number');
    expect(PARAM_META[PARAMS.DELAY_TIME].unit).toBe('ms');
    expect(PARAM_META[PARAMS.DELAY_FEEDBACK].max).toBe(95);
    expect(PARAM_META[PARAMS.DELAY_MIX].unit).toBe('%');
  });

  it('registers limiter threshold parameter', () => {
    expect(PARAMS.LIMITER_THRESHOLD).toBeTypeOf('number');
    expect(PARAM_META[PARAMS.LIMITER_THRESHOLD].max).toBe(100);
  });
});
