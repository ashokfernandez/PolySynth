# Trademark and Brand Name Policy

## Purpose

This document establishes guidelines for avoiding trademarked or copyrighted brand names in SEA_DSP documentation, code comments, and commit messages.

## Prohibited Terms

The following terms are trademarked by their respective owners and should **NOT** be used in:
- Code comments
- Documentation (README, guides, etc.)
- Variable names or identifiers
- Commit messages
- Issue titles or descriptions

### Synthesizer Brands

**Avoid:**
- Moog (Moog Music Inc.)
- Prophet (Sequential Circuits / Dave Smith Instruments)
- Minimoog
- Prophet-5, Prophet-10
- Roland (Roland Corporation)
- Korg (Korg Inc.)
- Yamaha (Yamaha Corporation)

**Use Instead:**
- "Classic 4-pole ladder filter"
- "Vintage transistor ladder topology"
- "Cascaded state-variable stages"
- "Multi-mode resonant filter"
- "Classic analog filter design"

### Effects & Technology Brands

**Avoid:**
- Dolby
- dbx
- BBD (if referring to bucket-brigade devices by brand)

**Use Instead:**
- "Analog delay line"
- "Compander circuit"
- "Noise reduction system"

## Acceptable Usage

You **MAY** reference trademarked terms when:

1. **Academic Citations**: Referencing published papers or patents
   - Example: "Implementation based on techniques described in Zavalishin (2012)"

2. **Historical Context**: When absolutely necessary for historical accuracy in documentation
   - Must include disclaimer: "All trademarks are property of their respective owners"
   - Use sparingly and only when generic description is insufficient

3. **Configuration/Compatibility**: When describing interoperability
   - Example: "Compatible with DaisySP library" (factual statement)

## Generic Alternatives

| Trademarked Term | Generic Alternative |
|------------------|---------------------|
| Moog filter | 4-pole ladder filter, transistor ladder filter |
| Prophet filter | Cascaded state-variable filter, dual-SVF cascade |
| Minimoog oscillator | Classic analog oscillator with multiple waveforms |
| Roland-style | Japanese vintage synthesizer topology |
| Buchla | West-coast synthesis approach |
| ARP | Classic American synthesizer design |

## Technical Descriptions

When describing filter characteristics inspired by classic designs, focus on **technical specifications**:

### Good Examples:
- "24dB/octave ladder topology with transistor-based saturation modeling"
- "Cascaded 12dB state-variable stages with feedback path"
- "4-pole resonant lowpass using TPT integrators"
- "Dual-stage SVF with adjustable slope (12/24 dB/octave)"

### Avoid:
- "Moog-style filter"
- "Prophet-5 cascade"
- "Minimoog sound"

## Code Comments

### ❌ Incorrect:
```cpp
// Moog ladder filter implementation
// Based on Prophet-5 cascade design
```

### ✅ Correct:
```cpp
// 4-pole ladder filter with transistor model
// Dual-stage cascaded SVF topology
```

## Documentation

### ❌ Incorrect:
> SEA_DSP includes a Moog ladder filter for that classic analog warmth.

### ✅ Correct:
> SEA_DSP includes a 4-pole transistor ladder filter emulating classic analog resonant filtering characteristics.

## Commit Messages

### ❌ Incorrect:
```
feat: add Moog-style ladder filter
fix: Prophet cascade filter resonance bug
```

### ✅ Correct:
```
feat: add 4-pole ladder filter with transistor model
fix: cascade filter resonance calculation
```

## Enforcement

- All pull requests will be reviewed for trademark compliance
- Automated checks may flag common trademarked terms
- Contributors are expected to self-review before submitting

## Questions?

If you're unsure whether a term is trademarked or how to describe something generically:

1. Search the term on the USPTO trademark database (https://www.uspto.gov/trademarks)
2. Focus on the **technical characteristics** rather than brand association
3. When in doubt, use generic technical terminology

## Legal Disclaimer

All product names, logos, brands, and trademarks mentioned in this document are the property of their respective owners. Use of these names does not imply endorsement or affiliation.

This policy is not legal advice. For trademark law questions, consult a qualified attorney.

---

**Last Updated**: 2024-02
**Policy Version**: 1.0
