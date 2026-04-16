/*! \page gmm_syntax Gorgon Music Macro (GMM) Syntax

Gorgon Music Macro (GMM) is a compact text format for describing polyphonic, retro-style music and sound effects directly as strings. It is designed to be easy to read and write while still supporting advanced features like multiple voices, multi-channel panning, tempo automation, instrument switching, and non-linear parameter ramps.

GMM separates the definition of global settings and instruments (the *header*) from the actual musical sequence (the *body*). This split keeps the playback code free of hard-coded “magic numbers” and makes it easy to reuse instrument settings across multiple tracks.

GMM supports comments through the `#` symbol. Anything following `#` on a line is ignored, allowing you to annotate your music or temporarily disable parts of the sequence. 

**Note: The following features are currently incomplete:**
- Slide
- Tempo and volume ramps
- Tracks
- Vibrato

---

## 1. File Structure

### Header (Engine Config & Instrument Declarations)
The header consists of global engine configurations (starting with `%`) and instrument declarations (starting with `@`). Instrument declarations assign a numeric ID to a waveform type and its envelope.

### Body (Track Data)
The body contains one or more tracks, each tagged with a track identifier (`1>`, `2>`, …). The tracks are processed simultaneously, allowing polyphony.

**Example File:**
```text
# --- Engine Config ---
%CHANNELS = 2

# --- Instrument Bank ---
@1 = sine(Flute), attack={s, 64}, decay={linear, 2/1}, sustain=0, release={exp, 4}
@2 = pulse(Lead), duty=50
@3 = noise(Snare), bitdepth=8

# --- Sequence Data ---
1> T120 @1 C4 D4 E3/4 R4 G2.
2> @2 C2 C2 C2 C2

# Multi-channel panning, volume fades, and non-linear slides
1> V[0]100 V[1]0 C4 V[0]0:4{exp} V[1]100:4{exp} C4^F4{sqrt} V0 R4
```

---

## 2. Core Commands

### Engine Configuration
* **`%KEY = value`**: Global engine configuration. Must be defined in the header.
    * `%CHANNELS = 2` sets the engine to render interleaved stereo audio.
    * `%CHANNELS = [FL, FR]` explicitly specifies channels (See Gorgon::Audio::Channel for details).

### Track & Playback Control
* **`N>`**: Track identifier. Maps the following sequence to a specific voice (e.g., `1>`, `2>`). Each track is mixed together. Tracks can be split across multiple lines for readability. If track identified is not specified, it defaults to `1>`. Tracks have their own tempo and volume settings, but share the same global engine configuration (e.g., channels) and instrument bank.
* **`T<Value>[:Duration]{Curve}`**: Tempo change in beats per minute. Can be immediate (`T120`) or ramped (`T60:1{s}`). Affects only the current track.
* **`V<Percent>[:Duration]{Curve}`**: Global track volume (0–100%). Modifies all channels for the current track.
* **`V[<Channel>]<Percent>[:Duration[{<Curve>}]]`**: Channel-specific volume override for panning. Example: `V[0]100` sets Channel 0 to 100%. `V[1]0:2` fades Channel 1 to 0% over a half-note.

### Musical Notation
* **`A`–`G`**: Musical notes. Can be followed by `+` (sharp) or `-` (flat). If no duration is specified, a quarter note is used.
* **`R<Duration>`**: Rest (silence) for the given duration.
* **`O<Octave>` / `<` / `>`**: Octave control. `O5` sets octave 5. `<` and `>` shift the current octave down or up by one.
* **`~[{Rate, Depth, Delay, End, Curve}]` (Vibrato)**: Applies pitch modulation to the preceding note to simulate a vibrating string or breath.
    * If used alone (`C4~`), it applies the instrument's default vibrato settings.
    * Can be overridden inline with tuple parameters: `C4~{5.0, 0.5, 0}` (5Hz rate, half-semitone depth, no delay).
* **`^` (Slide/Portamento)**: A single command that smoothly glides pitch from a starting note to a target note over a specified duration.
    * **Syntax:** `<StartNote>^<TargetNote><Duration>[{Curve}]`
    * **Example:** `C4^G4:2` slides from C4 to G4 over a half note.
    * **Example:** `C4^C5:4{s}` slides an entire octave over a quarter note using an S-Curve for a smooth takeoff and landing.
* **`S<Duration>`**: Sets note separation (articulation). Controls how long before the end of the note the release phase is triggered. Example: `S8` triggers the release an eighth note before the note ends. This allows for more staccato or legato phrasing by controlling how much of the release ramp is audible. `S` without a duration returns to the default separation defined in the instrument.

---

## 3. Durations & Ramps

### Durations
Duration tokens can be applied to notes, rests, and transitions.
* `2` — Fraction of a whole note (e.g., half note).
* `.` — Appended to a fraction to extend it by 50% (dotted note).
* `3/4` — Explicit fractional duration.
* `(0.5)` — Absolute time in seconds.
* `0.22` — Absolute tempo units (relative to current BPM).
* `[0.5]` — Fraction of the current note's length (used only in instrument settings).

### Ramps & Curves
Ramps govern how values transition over time (tempo, volume, and envelopes). They are defined by a Type, a Duration, and an optional Shape Factor. Parameters are separated by commas. Ramps are mirrored in the temporal domain if used as a falloff.

**Ramp Types:**
* `none`: Direct transition.
* `linear`: Standard linear transition.
* `exp` (Exponential): Starts slowly but aggressively ramps to the target (can cause clicks).
* `sqrt` (Square Root): Starts aggressively but slows down near the target.
* `log` (Logarithmic): Like square root, but more aggressive.
* `s` (SCurve): Soft start and stop for the smoothest transition.

If the type is not `none`, a duration must be specified. If the type is not `linear`, a shape factor (0 = linear, close to 1 = extreme curve) can be specified. Shape must be in the range [0, 1). Do not use values too close to 1 as they can cause extreme curves that may cause audio artifacts.

---

## 4. Instruments

Instruments are defined in the header and can be assigned inline using `@ID`. Each instrument has a waveform type and optional envelope parameters. A custom name can be specified in `()` immediately after the type for readability. All ramps default to S curve with a shape factor of 0.5, if only the duration is specified. None can also be used without `{}` for attack, decay, and release to set the ramp to be none. Example:
```text
@1 = sine (Flute) attack=none, decay=none, sustain=0

### Sine
A pure sine wave generator. With a smooth release, it mimics a flute.
* **`attack`**: Controls how aggressively the note's volume increases (`none` can cause clicks). Specified as a ramp.
* **`decay`**: Controls how quickly the sound fades after the attack phase (`none` sustains the note at full volume; other types cause it to fade before the note ends). Specified as a ramp.
* **`sustain`**: The volume multiplier (0.0 to 1.0) held after the attack and decay phases.
* **`release`**: Controls how quickly the sound fades after the sustain phase. Triggered by the `S` separation value (`none` abruptly ends the note). Specified as a ramp.

**Example Definition:**
```text
@1 = sine(Guitar), attack=64, decay={linear, 2/1}, sustain=0, release={exp, 4}
```

**Vibrato Settings**
Instruments can define a default vibrato profile. This allows notes to organically modulate without needing track-level modifiers. It requires three parameters defined as a tuple: `{Rate, Depth, Delay}`.
* **`Rate`**: The speed of the wobble in Hz (e.g., `5.0` to `7.0` is typical for acoustic instruments).
* **`Depth`**: The maximum pitch deviation in semitones (e.g., `0.5` is a quarter-step bend).
* **`Delay`**: Absolute time in seconds (e.g., `(0.2)`) or duration fraction (e.g., `8`) before the vibrato effect begins, mimicking a human player settling on a note before applying vibrato.

**Example Definition:**
```text
@1 = sine(Violin), attack={s, 32}, vibrato={6.0, 0.25, 16}
```
*/