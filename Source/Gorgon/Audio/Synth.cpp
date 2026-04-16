#include "Synth.h"
#include "Gorgon/Audio/Basic.h"
#include "Gorgon/String.h"
#include "Gorgon/Types.h"
#include "Gorgon/Utils/Assert.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <mutex>
#include <string>
#include <tuple>
#include <iostream>

namespace Gorgon :: Audio {

namespace internal {

    struct ADSRInfo {
        double attack,
               decay,
               sustain,
               release;
    };

    /// Common function to calculate durations of attack, decay
    /// sustain and release phases in samples for a note.
    ADSRInfo CalculateADSR(
        const Synth::Ramp &attack, const Synth::Ramp &decay, const Synth::Ramp &release, 
        const Synth::Duration &separation,
        float tempo, unsigned int sample_rate, float notelength
    ) {
        double length = notelength * sample_rate;

        double sep = separation.ToSamples(tempo, sample_rate, notelength);

        if(sep > length) {
            sep = length;
        }

        double atk = attack.ToSamples(
            tempo, sample_rate, 
            notelength
        );

        if(atk > length - sep) {
            atk = length - sep;
        }

        double dec = decay.ToSamples(tempo, sample_rate, notelength);

        if(dec > length - atk - sep) {
            dec = length - atk - sep;
        }

        double rel = release.ToSamples(tempo, sample_rate, notelength);

        double sus = length - atk - sep - dec;

        return {atk, dec, sus, rel};
    }

    /// Common overflow calculation from attack and release ramps for a note.
    double CalculateOverflow(
        const Synth::Ramp &attack, const Synth::Ramp &decay, const Synth::Ramp &release, 
        const Synth::Duration &separation,
        float tempo, unsigned int sample_rate, const Synth::Node &note
    ) {
        if(release.Type == Synth::RampType::None) {
            return 0;
        }

        auto notelength = note.note.duration.ToSeconds(tempo);
        double total = note.note.duration.ToSamples(tempo, sample_rate);
        auto [atk, dec, sus, rel] = CalculateADSR(attack, decay, release, separation, tempo, sample_rate, notelength);

        auto sep = total - atk - dec - sus;

        if(rel < sep) {
            rel = 0;
        }
        else {
            rel -= sep;
        }

        return rel;
    }

}

///// Node FUNCTIONS /////

Synth::Node Synth::Node::MakeNote(Note note, Duration duration, bool slide) {
    Node n;

    n.type = Type::Note;
    n.note.note = note;
    n.note.duration = duration;
    n.note.slide = slide;

    return n;
}

Synth::Node Synth::Node::MakeRest(Duration duration) {
    Node n;

    n.type = Type::Rest;
    n.note.duration = duration;

    return n;
}

Synth::Node Synth::Node::MakeTempo(float tempo) {
    Node n;

    n.type = Type::Tempo;
    n.tempo = tempo;

    return n;
}

Synth::Node Synth::Node::MakeOctaveAbsolute(int octave) {
    Node n;

    n.type = Type::OctaveAbsolute;
    n.octave = octave;

    return n;
}

Synth::Node Synth::Node::MakeOctaveRelative(int delta) {
    Node n;

    n.type = Type::OctaveRelative;
    n.octave = delta;

    return n;
}

Synth::Node Synth::Node::MakeVolume(float volume, int channel) {
    Node n;

    n.type = Type::Volume;
    n.volume.volume = volume;
    n.volume.channel = channel;

    return n;
}

Synth::Node Synth::Node::MakeSeparation(Duration duration) {
    Node n;

    n.type = Type::Separation;
    n.duration = duration;

    return n;
}

Synth::Node Synth::Node::MakeInstrumentChange(size_t instrument_index) {
    Node n;

    n.type = Type::InstrumentChange;
    n.index = instrument_index;

    return n;
}

///// DURATION FUNCTIONS /////

Synth::Duration Synth::Duration::FromFraction(int numerator, int denominator) {
  Duration d;
  d.type = TempoFraction;
  d.Fraction.Numerator = numerator;
  d.Fraction.Denominator = denominator;
  return d;
}

Synth::Duration Synth::Duration::FromUnits(float units) {
    Duration d;
    d.type = TempoUnits;
    d.Units = units;
    return d;
}

Synth::Duration Synth::Duration::FromSeconds(float seconds) {
    Duration d;
    d.type = ClockSeconds;
    d.Seconds = seconds;
    return d;
}

Synth::Duration Synth::Duration::FromNoteFraction(float fraction) {
    Duration d;
    d.type = NoteFraction;
    d.Units = fraction;
    return d;
}

Synth::Duration Synth::Duration::Parse(const std::string_view& token, bool allow_empty) {
    if(token.empty()) {
        if(allow_empty) return Duration::Empty();
        return Duration::FromFraction(4);
    }

    std::string normalized = String::ToLower(String::Trim(std::string{token}));

    if(normalized.find('/') != std::string::npos) {
        auto nominator = String::Extract(normalized, '/');
        if(nominator.empty()) {
            throw Error(Error::InvalidParameter, "Missing numerator in duration fraction: " + normalized);
        }
        if(normalized.empty()) {
            throw Error(Error::InvalidParameter, "Missing denominator in duration fraction: " + normalized);
        }
        int den;
        auto [nom, res] = String::FromCLocaleTo<int>(nominator);
        if(res == String::FromCLocaleToState::Failed) {
            throw Error(Error::InvalidParameter, "Invalid duration value: " + normalized);
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw Error(Error::InvalidParameter, "Extra characters after duration value: " + normalized);
        }
        if(nom < 0) {
            throw Error(Error::InvalidParameter, "Nominator cannot be zero in duration fraction: " + normalized);
        }

        std::tie(den, res) = String::FromCLocaleTo<int>(normalized);
        if(res == String::FromCLocaleToState::Failed) {
            throw Error(Error::InvalidParameter, "Invalid duration value: " + normalized);
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw Error(Error::InvalidParameter, "Extra characters after duration value: " + normalized);
        }
        if(den <= 0) {
            throw Error(Error::InvalidParameter, "Denominator cannot be zero in duration fraction: " + normalized);
        }
        return Duration::FromFraction(nom, den);
    }
    
    if(normalized[0] == '(') {
        if(normalized.back() != ')') {
            throw Error(Error::InvalidToken, "Mismatched parentheses in duration token: " + std::string{token});
        }

        auto [seconds, res] = String::FromCLocaleTo<float>(normalized.substr(1, normalized.size() - 2));
        if(res == String::FromCLocaleToState::Failed) {
            throw Error(Error::InvalidParameter, "Invalid duration value: " + normalized);
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {   
            throw Error(Error::InvalidParameter, "Extra characters after duration value: " + normalized);
        }

        if(seconds < 0) {
            throw Error(Error::InvalidParameter, "Duration in seconds cannot be negative: " + normalized);
        }

        return Duration::FromSeconds(seconds);
    }


    if(normalized[0] == '[') {
        if(normalized.back() != ']') {
            throw Error(Error::InvalidToken, "Mismatched brackets in duration token: " + std::string{token});
        }

        auto [units, res] = String::FromCLocaleTo<float>(normalized.substr(1, normalized.size() - 2));
        if(res == String::FromCLocaleToState::Failed) {
            throw Error(Error::InvalidParameter, "Invalid duration value: " + normalized);
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {   
            throw Error(Error::InvalidParameter, "Extra characters after duration value: " + normalized);
        }
        if(units < 0) {
            throw Error(Error::InvalidParameter, "Duration in units cannot be negative: " + normalized);
        }

        return Duration::FromNoteFraction(units);
    }

    // this last state is 3 in 1. If ends with dot, it's a fraction with 1.5 multiplier.
    // if it has a dot in the middle, it's unit based duration. Otherwise, it's a simple fraction.
    if(auto pos = normalized.find('.'); pos != std::string::npos) {
        if(pos == normalized.size() - 1) {
            auto [base, res] = String::FromCLocaleTo<int>(normalized.substr(0, pos));
            if(res == String::FromCLocaleToState::Failed) {
                throw Error(Error::InvalidParameter, "Invalid duration value: " + normalized.substr(0, pos));
            }
            if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
                throw Error(Error::InvalidParameter, "Extra characters after duration value: " + normalized.substr(0, pos));
            }
            if(base <= 0) {
                throw Error(Error::InvalidParameter, "Denominator cannot be zero in duration fraction: " + normalized.substr(0, pos));
            }

            return Duration::FromFraction(3, base * 2);
        }
        else {
            auto [units, res] = String::FromCLocaleTo<float>(normalized);
            if(res == String::FromCLocaleToState::Failed) {
                throw Error(Error::InvalidParameter, "Invalid duration value: " + normalized);
            }
            if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
                throw Error(Error::InvalidParameter, "Extra characters after duration value: " + normalized);
            }
            if(units < 0) {
                throw Error(Error::InvalidParameter, "Duration in units cannot be negative: " + normalized);
            }

            return Duration::FromUnits(units);
        }
    }
    else {
        auto [den, res] = String::FromCLocaleTo<int>(normalized);
        if(res == String::FromCLocaleToState::Failed) {
            throw Error(Error::InvalidParameter, "Invalid duration value: " + normalized);
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw Error(Error::InvalidParameter, "Extra characters after duration value: " + normalized);
        }
        if(den <= 0) {
            throw Error(Error::InvalidParameter, "Denominator cannot be zero in duration fraction: " + normalized);
        }
        return Duration::FromFraction(den);
    }

    throw Error(Error::InvalidToken, "Unrecognized duration token: " + std::string{token});
}

float Synth::Duration::ToSeconds(float tempo) const {
    switch(type) {
    case TempoFraction:
        return 240.0f / tempo * Fraction.Numerator / Fraction.Denominator;
    case TempoUnits:
        return 240.0f / tempo * Units;
    case ClockSeconds:
        return Seconds;
    default:
        throw Error(Error::InvalidDuration, "Unsupported duration type");
    }
}

double Synth::Duration::ToSamples(float tempo, unsigned int sample_rate) const {
    switch(type) {
    case TempoFraction:
        return 240.0 / tempo * Fraction.Numerator / Fraction.Denominator * sample_rate;
    case TempoUnits:
        return 240.0 / tempo * Units * sample_rate;
    case ClockSeconds:
        return Seconds * sample_rate;
    default:
        throw Error(Error::InvalidDuration, "Unsupported duration type");
    }
}

float Synth::Duration::ToSeconds(float tempo, float notelength) const {
    switch(type) {
    case TempoFraction:
        return 240.0f / tempo * Fraction.Numerator / Fraction.Denominator;
    case TempoUnits:
        return 240.0f / tempo * Units;
    case ClockSeconds:
        return Seconds;
    case NoteFraction:
        return notelength * Units;
    default:
        throw Error(Error::InvalidDuration, "Unsupported duration type");
    }
}

double Synth::Duration::ToSamples(float tempo, unsigned int sample_rate, float notelength) const {
    switch(type) {
    case TempoFraction:
        return 240.0 / tempo * Fraction.Numerator / Fraction.Denominator * sample_rate;
    case TempoUnits:
        return 240.0 / tempo * Units * sample_rate;
    case ClockSeconds:
        return double(Seconds) * sample_rate;
    case NoteFraction:
        return double(notelength) * Units * sample_rate;
    default:
        throw Error(Error::InvalidDuration, "Unsupported duration type");
    }
}

///// RAMP FUNCTIONS /////

Synth::Ramp Synth::Ramp::Parse(const std::string_view &token) {
    std::string normalized = String::ToLower(String::Trim(std::string{token}));

    auto type = String::Trim(String::Extract(normalized, ','));

    RampType parsedtype = (RampType)-1; //invalid default value to trigger error
    parsedtype = String::To<RampType>(type);

    if(parsedtype == (RampType)-1) {
        throw Error(Error::InvalidParameter, "Unrecognized ramp type: " + type);
    }

    Ramp ramp = {
        parsedtype,
    };  

    if(parsedtype == RampType::None) {
        if(!normalized.empty()) {
            throw Error(Error::InvalidParameter, "Ramp type None should not have additional parameters: " + normalized);
        }
    }
    else {
        auto duration = String::Trim(String::Extract(normalized, ','));
        ramp.Span = Duration::Parse(duration);
    }

    if(!normalized.empty()) {
        if(parsedtype == RampType::Linear) {
            throw Error(Error::InvalidParameter, "Ramp type Linear should not have shape factor: " + normalized);
        }
        
        auto [shape, state] = String::FromCLocaleTo<float>(normalized);
        if(state == String::FromCLocaleToState::Failed) {
            throw Error(Error::InvalidParameter, "Invalid ramp shape value: " + normalized);
        }
        if(state == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw Error(Error::InvalidParameter, "Extra characters after ramp shape value: " + normalized);
        }

        if(shape < 0.0f || shape >= 1.0f) {
            throw Error(Error::InvalidParameter, "Ramp shape factor must be in the range [0, 1): " + normalized);
        }

        ramp.ShapeFactor = shape;
    }
    
    return ramp;
}

float Synth::Ramp::GetMultiplier(float t) const {
    if(Type == RampType::None) return 1.0f;
    if(t >= 1.0f) return 1.0f;
    if(ShapeFactor == 0.0f || Type == RampType::Linear) return t;

    auto p = 1.0f - ShapeFactor;

    switch(Type) {
    case RampType::Exponential:
        return std::pow(t, 1.0f / p);
    case RampType::SquareRoot:
        return std::pow(t, p);
    case RampType::Logarithmic:
        return (std::pow(p, t) - 1.0f) / (p - 1.0f);
    case RampType::SCurve: {
        if(p == 0.5f) return t * t * (3.0f - 2.0f * t);
        auto tp = std::pow(t, 1.0f / p);
        return tp / (tp + std::pow(1.0f - t, 1.0f / p));
    }
    default:
        throw Error(Error::InvalidParameter, "Unsupported ramp type");
    }
}

float Synth::Ramp::GetMultiplier(size_t distance, float tempo, unsigned int sample_rate, float note_length) const {
    if(Type == RampType::None) {
        return 1.0f;
    }

    auto total = Span.ToSamples(tempo, sample_rate, note_length);

    if(total == 0) return 1;

    float t = float(distance) / float(total);

    return GetMultiplier(t);
}

double Synth::Ramp::ToSamples(float tempo, unsigned int sample_rate, float note_length) const {
    if(Type == RampType::None) {
        return 0;
    }
    
    return Span.ToSamples(tempo, sample_rate, note_length);
}

///// SINE FUNCTIONS /////

void Synth::Sine::LoadSettings(const std::string_view &settings) {
    std::string normalized = String::ToLower(String::Trim(std::string{settings}));

    auto data = String::Map_UseQuotesAndParentheses(
        normalized, 
        '=', 
        ",",
        String::QuoteType::None,
        "({",
        ")}",
        true, true, true
    );

    for(const auto& [key, value] : data) {
        if(key == "attack") {
            if(value.empty()) {
                throw Error(Error::InvalidParameter, "Attack parameter cannot be empty");
            }

            if(value[0] == '{') {
                if(value.back() != '}') {
                    throw Error(Error::InvalidToken, "Mismatched braces in attack parameter: " + value);
                }

                Attack = Ramp::Parse(value.substr(1, value.size() - 2));
            }
            else if(value == "none") {
                Attack.Type = RampType::None;
            }
            else {
                Attack.Type = RampType::SCurve;
                Attack.Span = Duration::Parse(value);
            }
        }
        else if(key == "decay") {
            if(value.empty()) {
                throw Error(Error::InvalidParameter, "Decay parameter cannot be empty");
            }

            if(value[0] == '{') {
                if(value.back() != '}') {
                    throw Error(Error::InvalidToken, "Mismatched braces in decay parameter: " + value);
                }
                Decay = Ramp::Parse(value.substr(1, value.size() - 2));
            }
            else if(value == "none") {
                Decay.Type = RampType::None;
            }
            else {
                Decay.Type = RampType::SCurve;
                Decay.Span = Duration::Parse(value);
            }
        }
        else if(key == "release") {
            if(value.empty()) {
                throw Error(Error::InvalidParameter, "Release parameter cannot be empty");
            }

            if(value[0] == '{') {
                if(value.back() != '}') {
                    throw Error(Error::InvalidToken, "Mismatched braces in release parameter: " + value);
                }
                Release = Ramp::Parse(value.substr(1, value.size() - 2));
            }
            else if(value == "none") {
                Release.Type = RampType::None;
            }
            else {
                Release.Type = RampType::SCurve;
                Release.Span = Duration::Parse(value);
            }
        }
        else if(key == "sustain") {
            auto [sustain, state] = String::FromCLocaleTo<float>(value);
            if(state == String::FromCLocaleToState::Failed) {
                throw Error(Error::InvalidParameter, "Invalid sustain value: " + value);
            }
            if(state == String::FromCLocaleToState::ScrapAtTheEnd) {
                throw Error(Error::InvalidParameter, "Extra characters after sustain value: " + value);
            }
            Sustain = sustain;
        }
        else if(key == "separation") {
            Separation = Duration::Parse(value);
        }
        else if(key == "volume") {
            auto [volume, state] = String::FromCLocaleTo<float>(value);
            if(state == String::FromCLocaleToState::Failed) {
                throw Error(Error::InvalidParameter, "Invalid volume value: " + value);
            }
            if(state == String::FromCLocaleToState::ScrapAtTheEnd) {
                throw Error(Error::InvalidParameter, "Extra characters after volume value: " + value);
            }

            Volume = volume / 100.0f;
        }
        else if(key == "pitchoffset") {
            auto [offset, state] = String::FromCLocaleTo<float>(value);
            if(state == String::FromCLocaleToState::Failed) {
                throw Error(Error::InvalidParameter, "Invalid pitch offset value: " + value);
            }
            if(state == String::FromCLocaleToState::ScrapAtTheEnd) {
                throw Error(Error::InvalidParameter, "Extra characters after pitch offset value: " + value);
            }

            PitchOffset = offset;
        }
        else {
            throw Error(Error::InvalidParameter, "Unknown parameter for Sine instrument: " + key);
        }
    }
}

double Synth::Sine::ReleaseOverflow(TrackState &state, unsigned int sample_rate, const Node &note) const {
    return internal::CalculateOverflow(
        Attack, Decay, Release, state.Separation.Or(Separation), 
        state.Tempo, sample_rate, note
    );
}

double Synth::Sine::Render(Containers::Wave &wave, const Node &node, TrackState &state, unsigned int sample_rate) {
    float frequency = NoteToFrequency(node.note.note, state.Octave, PitchOffset);

    double phase = 0.0;
    double phasechange = double(frequency) / sample_rate;
    
    auto [atk, dec, sus, rel] = internal::CalculateADSR(
        Attack, Decay, Release, 
        state.Separation.Or(Separation), state.Tempo,
        sample_rate, 
        node.note.duration.ToSeconds(state.Tempo)
    );

    size_t start = size_t(std::round(state.Sample));
    size_t end = size_t(std::round(state.Sample + atk));

    float tchange = 1.0f / (end - start);
    float t = 0;
    float env = 0.0f;

    for(size_t i=start; i<end; i++) {
        env = Attack.GetMultiplier(t);

        for(unsigned ch = 0; ch < wave.GetChannelCount(); ch++) {
            float sample = std::sin(float(2.0 * Gorgon::PI * phase));
            wave(i, ch) +=  env * sample * Volume * state.Volume[ch];
        }

        phase += phasechange;
        if(phase >= 1.0) phase -= 1.0;
        t += tchange;
    }

    start = end;
    end = size_t(std::round(state.Sample + atk + dec));
    tchange = 1.0f / (end - start);
    t = 0;

    for(size_t i=start; i<end; i++) {
        env = 1.0f - Decay.GetMultiplier(t) * (1.0f - Sustain);

        for(unsigned ch = 0; ch < wave.GetChannelCount(); ch++) {
            float sample = std::sin(float(2.0 * Gorgon::PI * phase));
            wave(i, ch) +=  env * sample * Volume * state.Volume[ch];
        }

        phase += phasechange;
        if(phase >= 1.0) phase -= 1.0;
        t += tchange;
    }

    start = end;
    end = size_t(std::round(state.Sample + atk + dec + sus));
    tchange = 1.0f / (end - start);
    t = 0;

    for(size_t i=start; i<end; i++) {
        for(unsigned ch = 0; ch < wave.GetChannelCount(); ch++) {
            float sample = std::sin(float(2.0 * Gorgon::PI * phase));
            wave(i, ch) +=  env * sample * Volume * state.Volume[ch];
        }

        phase += phasechange;
        if(phase >= 1.0) phase -= 1.0;
        t += tchange;
    }

    start = end;
    end = size_t(std::round(state.Sample + atk + dec + sus + rel));
    tchange = 1.0f / (end - start);
    t = 0;
    auto sustain = env; // sustain level at the start of release phase

    for(size_t i=start; i<end; i++) {
        env = sustain - Release.GetMultiplier(t) * sustain;

        for(unsigned ch = 0; ch < wave.GetChannelCount(); ch++) {
            float sample = std::sin(float(2.0 * Gorgon::PI * phase))    ;
            wave(i, ch) +=  env * sample * Volume * state.Volume[ch];
        }

        phase += phasechange;
        if(phase >= 1.0) phase -= 1.0;
        t += tchange;
    }
    
    return node.note.duration.ToSamples(state.Tempo, sample_rate);
}

///// SYNTH FUNCTIONS /////

Synth::Node Synth::ParseNode(const std::string_view& token, int channels) {
    std::string normalized = String::ToLower(String::Trim(std::string{token}));

    if(normalized.empty()) {
        return {};
    }
        
    switch(normalized[0]) {
    case 't': {
        auto [tempo, res] = String::FromCLocaleTo<float>(normalized.substr(1));
        if(res == String::FromCLocaleToState::Failed) {
            throw Error(Error::InvalidParameter, "Invalid tempo value: " + normalized.substr(1));
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw Error(Error::InvalidParameter, "Extra characters after tempo value: " + normalized.substr(1));
        }
        return Node::MakeTempo(tempo);
    }
    case 'o': {
        auto [oct, res] = String::FromCLocaleTo<int>(normalized.substr(1));
        if(res == String::FromCLocaleToState::Failed) {
            throw Error(Error::InvalidParameter, "Invalid octave value: " + normalized.substr(1));
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw Error(Error::InvalidParameter, "Extra characters after octave value: " + normalized.substr(1));
        }
        return Node::MakeOctaveAbsolute(oct);
    }
    case '>':
        return Node::MakeOctaveRelative(1);
    case '<':
        return Node::MakeOctaveRelative(-1);
    case 'v': {
        if(normalized.size() < 2) {
            throw Error(Error::InvalidParameter, "Volume token must have a value: " + normalized);
        }

        int channel = 0;
        if(normalized[1] == '{') {
            auto endBrace = normalized.find('}');

            if(endBrace == std::string::npos) {
                throw Error(Error::InvalidParameter, "Missing closing brace for volume channel: " + normalized);
            }

            String::FromCLocaleToState res;
            auto channelstr = normalized.substr(2, endBrace - 2);
            std::tie(channel, res) = String::FromCLocaleTo<int>(channelstr);

            if(res == String::FromCLocaleToState::Failed) {
                throw Error(Error::InvalidParameter, "Invalid volume channel: " + channelstr);
            }
            if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
                throw Error(Error::InvalidParameter, "Extra characters after volume channel: " + channelstr);
            }

            if(channel < 0) {
                throw Error(Error::InvalidParameter, "Volume channel cannot be negative: " + std::to_string(channel));
            }
            else if(channel > channels) {
                throw Error(Error::InvalidParameter, "Volume channel index out of range: " + std::to_string(channel));
            }

            normalized.erase(1, endBrace);
        }
        
        auto [vol, res] = String::FromCLocaleTo<float>(normalized.substr(1));

        if(res == String::FromCLocaleToState::Failed) {
            throw Error(Error::InvalidParameter, "Invalid volume value: " + normalized.substr(1));
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw Error(Error::InvalidParameter, "Extra characters after volume value: " + normalized.substr(1));
        }
        if(vol < 0 || vol > 100) {
            throw Error(Error::InvalidParameter, "Volume must be between 0 and 100: " + std::to_string(vol));
        }

        return Node::MakeVolume(vol / 100.0f, channel);
    }

    case 'r':
        return Node::MakeRest(Duration::Parse(normalized.substr(1)));

    case 's':
        return Node::MakeSeparation(Duration::Parse(normalized.substr(1), true));
    
    case '@':{
        auto [inst, res] = String::FromCLocaleTo<size_t>(normalized.substr(1));
        if(res == String::FromCLocaleToState::Failed) {
            throw Error(Error::InvalidParameter, "Invalid instrument index: " + normalized.substr(1));
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw Error(Error::InvalidParameter, "Extra characters after instrument index: " + normalized.substr(1));
        }
        return Node::MakeInstrumentChange(inst);
    
    }
    default:
        if(normalized[0] >= 'a' && normalized[0] <= 'g') {
            Note note = Note::C;

            switch(normalized[0]) {
            case 'c': note = Note::C; break;
            case 'd': note = Note::D; break;
            case 'e': note = Note::E; break;
            case 'f': note = Note::F; break;
            case 'g': note = Note::G; break;
            case 'a': note = Note::A; break;
            case 'b': note = Note::B; break;
            default:
                throw Error(Error::InvalidToken, "Invalid note: " + std::string{normalized[0]});
            }

            bool slide = false;

            if(!normalized.empty() && normalized.back() == '~') {
                slide = true;
                normalized.pop_back();
            }

            int off = 1;

            // check sharp + and flat -
            if(normalized.size() > 1) {
                if(normalized[1] == '+') {
                    if(note == Note::B) {
                        throw Error(Error::InvalidParameter, "Cannot apply sharp to note B: " + std::string{token});
                    }
                    note = static_cast<Note>((static_cast<int>(note) + 1) % 12);
                    off = 2;
                }
                else if(normalized[1] == '-') {
                    if(note == Note::C) {
                        throw Error(Error::InvalidParameter, "Cannot apply flat to note C: " + std::string{token});
                    }
                    note = static_cast<Note>((static_cast<int>(note) + 11) % 12);
                    off = 2;
                }
            }

            return Node::MakeNote(note, Duration::Parse(normalized.substr(off)), slide);
        }

        throw Error(Error::InvalidToken, "Unrecognized token: " + std::string{token});
    }
}

void Synth::Parse(std::istream &stream) {
    auto guard = std::lock_guard(critical);

    Nodes.clear();

    instruments.DeleteAll();
    instruments = {
        instrumentfactories["sine"]()
    };

    std::string line;

    while(std::getline(stream, line)) {
        // remove comments
        auto commentPos = line.find('#');
        if(commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        line = String::Trim(line);
        if(line.empty()) continue;

        // render variables
        if(line[0] == '%') {
            line = String::Trim(line.substr(1));
            auto var = String::Trim(String::ToLower(String::Extract(line, '=')));
            line = String::Trim(line);

            if(var == "channels") {
                std::vector<Audio::Channel> channels;
                std::string ch;

                if(line.empty()) {
                    throw Error(Error::InvalidParameter, "Channels variable cannot be empty");
                }

                if(line[0] == '[') {
                    if(line.back() != ']') {
                        throw Error(Error::InvalidParameter, "Mismatched brackets in channels variable: " + line);
                    }

                    line.pop_back();
                    line.erase(0, 1);

                    while(!line.empty()) {
                        ch = String::Trim(String::Extract(line, ','));

                        if(ch.empty()) {
                            throw Error(Error::InvalidParameter, "Empty channel name in channels variable");
                        }

                        Audio::Channel channel = Channel::Unknown;
                        channel = String::To<Audio::Channel>(ch);

                        if(channel == Channel::Unknown) {
                            throw Error(Error::InvalidParameter, "Unrecognized channel name: " + ch);
                        }

                        channels.push_back(channel);
                    }
                }
                else {
                    auto [channelcount, state] = String::FromCLocaleTo<int>(line);

                    if(state == String::FromCLocaleToState::Failed) {
                        throw Error(Error::InvalidParameter, "Invalid channel count: " + line);
                    }
                    if(state == String::FromCLocaleToState::ScrapAtTheEnd) {
                        throw Error(Error::InvalidParameter, "Extra characters after channel count: " + line);
                    }
                    if(channelcount < 1 || channelcount > 6) {
                        throw Error(Error::InvalidParameter, "Channel count must be between 1 and 6: " + std::to_string(channelcount));
                    }

                    switch(channelcount) {
                    case 1: 
                        channels.push_back(Audio::Channel::Mono); 
                        break;
                    case 2: 
                        channels.push_back(Audio::Channel::FrontLeft); 
                        channels.push_back(Audio::Channel::FrontRight); 
                        break;
                    case 3: 
                        channels.push_back(Audio::Channel::FrontLeft); 
                        channels.push_back(Audio::Channel::FrontRight);
                        channels.push_back(Audio::Channel::LowFreq);
                        break;
                    case 4:
                        channels.push_back(Audio::Channel::FrontLeft); 
                        channels.push_back(Audio::Channel::FrontRight);
                        channels.push_back(Audio::Channel::BackLeft);
                        channels.push_back(Audio::Channel::BackRight);
                        break;
                    case 5:
                        channels.push_back(Audio::Channel::FrontLeft); 
                        channels.push_back(Audio::Channel::FrontRight);
                        channels.push_back(Audio::Channel::BackLeft);
                        channels.push_back(Audio::Channel::BackRight);
                        channels.push_back(Audio::Channel::LowFreq);
                        break;
                    case 6:
                        channels.push_back(Audio::Channel::FrontLeft); 
                        channels.push_back(Audio::Channel::FrontRight);
                        channels.push_back(Audio::Channel::BackLeft);
                        channels.push_back(Audio::Channel::BackRight);
                        channels.push_back(Audio::Channel::Center);
                        channels.push_back(Audio::Channel::LowFreq);
                        break;
                    }
                }

                Channels = std::move(channels);
            }
            else {
                throw Error(Error::InvalidParameter, "Unrecognized variable: " + var);
            }

            continue;
        }

        // check if this is an instrument definition line but a starting @
        // is not enough as this could be a valid instrument change node. 
        // we will check if there is a = sign in the line to determine if
        // this is an instrument definition or not.
        if(line[0] == '@') {
            size_t pos;

            pos = line.find_first_of("= ");

            if(pos != std::string::npos) {
                //eat up space
                while(line[pos] == ' ' || line[pos] == '\t') {
                    pos++;
                }
            }

            // we reached to = sign, this is an instrument definition line
            if(pos != std::string::npos && line[pos] == '=') {
                auto indexstr = String::Trim(line.substr(1, pos - 1));
                auto [index, res] = String::FromCLocaleTo<size_t>(indexstr);

                std::string type, name;

                if(res == String::FromCLocaleToState::Failed) {
                    throw Error(Error::InvalidParameter, "Invalid instrument index: " + indexstr);
                }
                if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
                    throw Error(Error::InvalidParameter, "Extra characters after instrument index: " + indexstr);
                }
                if(index == 0) {
                    throw Error(Error::InvalidParameter, "Instrument index must be greater than 0: " + std::to_string(index));
                }
                index--; // make it 0 based index

                line = String::Trim(line.substr(pos + 1));
                
                if(line.empty()) {
                    throw Error(Error::InvalidParameter, "Instrument settings cannot be empty for instrument index: " + std::to_string(index));
                }

                pos = line.find_first_of(" \t(");

                type = String::ToLower((pos == std::string::npos) ? line : line.substr(0, pos));
                
                if(pos != std::string::npos) {
                    line = String::Trim(line.substr(pos));
                }
                else {
                    line = "";
                }

                if(!line.empty() && line[0] == '(') {
                    pos = line.find(')');
                    if(pos == std::string::npos) {
                        throw Error(Error::InvalidParameter, "Mismatched parentheses in instrument name for instrument index: " + std::to_string(index));
                    }

                    name = String::Trim(line.substr(1, pos - 1));

                    line = String::Trim(line.substr(pos + 1));
                }

                Instrument::Factory factory;

                if(auto inst = instrumentfactories.find(type); inst != instrumentfactories.end()) {
                    factory = inst->second;
                }
                else if(auto inst = baseinstrumentfactories.find(type); inst != baseinstrumentfactories.end()) {
                    factory = inst->second;
                }
                else {
                    throw Error(Error::UnknownInstrument, "Unrecognized instrument type: " + type);
                }

                auto &instr = factory();
                instr.Name = name;
                instr.LoadSettings(line);

                if(index == (size_t)instruments.GetSize()) {
                    instruments.Add(instr);
                }
                else if(index > (size_t)instruments.GetSize()) {
                    throw Error(Error::InvalidParameter, "Instrument index cannot have gaps: " + std::to_string(index));
                }
                else {
                    instruments.Replace(long(index), &instr, true);
                }

                continue;
            }
        }

        std::size_t pos = 0;

        while(pos != std::string::npos) {
            auto nextpos = line.find_first_of(" \t~", pos);

            std::string token = line.substr(pos, nextpos - pos);
            
            if(nextpos != std::string::npos && line[nextpos] == '~')
                token += '~';

            pos = (nextpos == std::string::npos) ? std::string::npos : nextpos + 1;

            if(String::Trim(token).empty()) continue;
            auto node = ParseNode(token);

            if(node.type == Node::Type::InstrumentChange) {
                if(node.index > (size_t)instruments.GetSize()) {
                    throw Error(Error::UnknownInstrument, "Instrument index out of range: " + std::to_string(node.index + 1));
                }
            }

            if(node.type != Node::Type::NoOp) {
                Nodes.push_back(node);
            }
        }
    }
}

Synth::AudioDuration Synth::CalculateSamples(unsigned int sample_rate) const {
    auto guard = std::lock_guard(critical);

    auto [total, end] = calculatesamples(sample_rate);
    return {static_cast<size_t>(std::ceil(total)), static_cast<size_t>(std::ceil(end))};
}

std::pair<double, double> Synth::calculatesamples(unsigned int sample_rate) const {
    TrackState state;

    double end = 0;

    for(const auto& node : Nodes) {
        switch(node.type) {
        case Node::Type::Note: {
            auto duration = node.note.duration.ToSamples(state.Tempo, sample_rate);
            auto overflow = state.InstrumentIndex == 0 ? 0 : instruments[long(state.InstrumentIndex) - 1].ReleaseOverflow(state, sample_rate, node);
            state.Sample += duration;
            end = std::max(end, state.Sample + overflow);
            break;
        }
        case Node::Type::Rest: {
            auto duration = node.note.duration.ToSamples(state.Tempo, sample_rate);
            state.Sample += duration;
            end = std::max(end, state.Sample);
            break;
        }
        case Node::Type::Tempo:
            state.Tempo = node.tempo;
            break;
        case Node::Type::Separation:
            state.Separation = node.duration;
            break;
        case Node::Type::InstrumentChange:
            state.InstrumentIndex = node.index;
            break;
        default:
            break;
        }
    }

    return {end, state.Sample};
}

float Synth::CalculateDuration() const { 
    return CalculateSamples(151200).Total / 151200.0f; 
}

Containers::Wave Synth::Render(unsigned int sample_rate) const {
    auto guard = std::lock_guard(critical);

    Containers::Wave wave(size_t(std::ceil(calculatesamples(sample_rate).first)), unsigned(sample_rate), Channels);
    wave.Clear();

    if(Channels != std::vector<Audio::Channel>{Audio::Channel::Mono}) {
        Utils::NotImplemented("Only mono output is supported currently");
    }

    TrackState state;
    state.Volume = std::vector<float>(Channels.size(), 1.0f);

    for(const auto &node: Nodes) {
        switch(node.type) {
        case Node::Type::Tempo:
            state.Tempo = node.tempo;
            break;

        case Node::Type::OctaveAbsolute:
            state.Octave = node.octave;
            break;

        case Node::Type::OctaveRelative:
            state.Octave += node.octave;
            break;

        case Node::Type::Volume:
            if(node.volume.channel == 0) {
                std::fill(state.Volume.begin(), state.Volume.end(), node.volume.volume);
            }
            else {
                state.Volume[node.volume.channel - 1] = node.volume.volume;
            }
            break;
        
        case Node::Type::Separation:
            state.Separation = node.duration;
            break;

        case Node::Type::InstrumentChange:
            state.InstrumentIndex = node.index;

            if(state.InstrumentIndex > size_t(instruments.GetSize())) {
                throw Error(Error::InvalidParameter, "Instrument index out of range: " + std::to_string(state.InstrumentIndex));
            }
            
            break;

        case Node::Type::Rest:
            state.Sample += node.note.duration.ToSamples(state.Tempo, sample_rate);
            break;
        case Node::Type::Note: {
            double duration;

            // Instrument index 0 is silent, we just advance the sample position without rendering anything
            if(state.InstrumentIndex == 0) {
                duration = node.note.duration.ToSamples(state.Tempo, sample_rate);
            }
            else {
                duration = instruments[long(state.InstrumentIndex) - 1].Render(wave, node, state, sample_rate);
            }

            state.Sample += duration;
            break;
        }

        case Node::Type::NoOp:
            break;
        }
    }

    return wave;
}

void Synth::AddInstrumentFactory(std::function<Instrument &()> factory, const std::string &name) {
  auto guard = std::lock_guard(critical);

  if(name.empty()) {
    throw Error(Error::InvalidParameter, "Instrument factory name cannot be empty");
  }
  if(!factory) {
    throw Error(Error::InvalidParameter, "Instrument factory cannot be empty");
  }

  instrumentfactories[String::ToLower(name)] = factory;
}

void Synth::RemoveInstrumentFactory(const std::string &name) {
  auto guard = std::lock_guard(critical);

  instrumentfactories.erase(String::ToLower(name));
}

void Synth::ClearInstrumentFactories() {
  auto guard = std::lock_guard(critical);

  instrumentfactories.clear();
  instrumentfactories["sine"] = []() -> Instrument & { return *new Sine(); };
}

size_t Synth::AddInstrument(Instrument& instrument) {
    auto guard = std::lock_guard(critical);
    
    instruments.Add(instrument);
    return size_t(instruments.GetSize());
}

void Synth::SetInstrument(size_t index, Instrument& instrument) {
    auto guard = std::lock_guard(critical);

    if(index == 0 || index > size_t(instruments.GetSize())) {
        throw Error(Error::InvalidParameter, "Instrument index out of range: " + std::to_string(index));
    }
    instruments.Replace(long(index - 1), &instrument, true);
}

const Synth::Instrument& Synth::GetInstrument(size_t index) const {
    if(index == 0 || index > size_t(instruments.GetSize())) {
        throw Error(Error::InvalidParameter, "Instrument index out of range: " + std::to_string(index));
    }
    return instruments[long(index - 1)];
}

size_t Synth::GetInstrumentCount() const {
    return size_t(instruments.GetSize());
}

void Synth::RemoveInstrument(size_t index) {
    auto guard = std::lock_guard(critical);

    if(index == 0 || index > size_t(instruments.GetSize())) {
        throw Error(Error::InvalidParameter, "Instrument index out of range: " + std::to_string(index));
    }
    instruments.Delete(long(index - 1));
}

void Synth::AddChannel(Audio::Channel channel) {
    auto guard = std::lock_guard(critical);

    if(HasChannel(channel)) return;

    Channels.push_back(channel);
    std::sort(Channels.begin(), Channels.end(), [](Audio::Channel a, Audio::Channel b) {
        return static_cast<int>(a) < static_cast<int>(b);
    });
}

bool Synth::HasChannel(Audio::Channel channel) const {
    return std::find(Channels.begin(), Channels.end(), channel) != Channels.end();
}

void Synth::RemoveChannel(Audio::Channel channel) {
    auto guard = std::lock_guard(critical);

    auto it = std::find(Channels.begin(), Channels.end(), channel);
    if(it != Channels.end()) {
        Channels.erase(it);
    }
}

void Synth::AddNode(Node node) {
    auto guard = std::lock_guard(critical);

    Nodes.push_back(node);
}

void Synth::RemoveNode(size_t index) {
    auto guard = std::lock_guard(critical);

    Nodes.erase(Nodes.begin() + index);
}

const std::map<std::string, Synth::Instrument::Factory> Synth::baseinstrumentfactories = {
    {"chiptune", []() -> Instrument& { 
        static Sine sine;
        sine.Name = "Chiptune";
        sine.Attack = {RampType::None};
        sine.Decay = {RampType::Linear, Synth::Duration::FromFraction(16)};
        sine.Sustain = 0.0f;
        sine.Release = {RampType::Linear, Synth::Duration::FromFraction(16)};
        sine.Volume = 1.0f;
        return sine.Clone(); 
    }},
    {"ambientpad", []() -> Instrument& { 
        static Sine sine;
        sine.Name = "Ambient Pad";
        sine.Attack = {RampType::Linear, Synth::Duration::FromFraction(2)};
        sine.Decay = {RampType::Linear, Synth::Duration::FromFraction(4)};
        sine.Sustain = 0.8f;
        sine.Release = {RampType::Linear, Synth::Duration::FromFraction(1)};
        sine.Volume = 0.4f;
        return sine.Clone(); 
    }},
    {"electricpiano", []() -> Instrument& { 
        static Sine sine;
        sine.Name = "Electric Piano";
        sine.Attack = {RampType::SCurve, Synth::Duration::FromSeconds(0.02f)};
        sine.Decay = {RampType::SCurve, Synth::Duration::FromSeconds(0.3f)};
        sine.Sustain = 0.3f;
        sine.Release = {RampType::SCurve, Synth::Duration::FromSeconds(0.6f)};
        return sine.Clone(); 
    }},
    {"deepsubbass", []() -> Instrument& { 
        static Sine sine;
        sine.Name = "Deep Sub Bass";
        sine.Attack = {RampType::Linear, Synth::Duration::FromSeconds(0.01f)};
        sine.Decay = {RampType::None};
        sine.Sustain = 1.0f;
        sine.Release = {RampType::SCurve, Synth::Duration::FromSeconds(0.15f)};
        sine.PitchOffset = -24.0f; // 2 octaves down
        sine.Volume = 0.7f;
        return sine.Clone(); 
    }},
    {"xylophone", []() -> Instrument& { 
        static Sine sine;
        sine.Name = "Xylaphone";
        sine.Attack = {RampType::SCurve, Synth::Duration::FromSeconds(0.01f)};
        sine.Decay = {RampType::SCurve, Synth::Duration::FromSeconds(0.6f)};
        sine.Sustain = 0.3f;
        sine.Release = {RampType::SCurve, Synth::Duration::FromSeconds(1.2f)};
        sine.Volume = 0.6f;
        sine.PitchOffset = 12.0f; // 1 octave up
        return sine.Clone(); 
    }},
    {"synth", []() -> Instrument& { 
        static Sine sine;
        sine.Name = "Synth";
        sine.Attack = {RampType::Linear, Synth::Duration::FromFraction(128)};
        sine.Decay = {RampType::Linear, Synth::Duration::FromFraction(16)};
        sine.Sustain = 0.8f;
        sine.Release = {RampType::Linear, Synth::Duration::FromFraction(16)};
        return sine.Clone(); 
    }},
    {"flute", []() -> Instrument& { 
        static Sine sine;
        sine.Name = "Flute";
        sine.Attack = {RampType::SCurve, Synth::Duration::FromSeconds(0.04f)};
        sine.Decay = {RampType::None};
        sine.Sustain = 1.0f;
        sine.Release = {RampType::Logarithmic, Synth::Duration::FromSeconds(0.06f)};
        sine.Separation = Synth::Duration::FromSeconds(0.2f);
        sine.Volume = 1.0f;
        return sine.Clone(); 
    }},
    {"bell", []() -> Instrument& { 
        static Sine sine;
        sine.Name = "Bell";
        sine.Attack = {RampType::SCurve, Synth::Duration::FromSeconds(0.01f)};
        sine.Decay = {RampType::Logarithmic, Synth::Duration::FromSeconds(1.5f)};
        sine.Sustain = 0.0f;
        sine.Release = {RampType::Logarithmic, Synth::Duration::FromSeconds(1.5f)};
        sine.Volume = 0.4f;
        sine.PitchOffset = 12.0f; // 1 octave up
        return sine.Clone(); 
    }},
};

} // namespace Gorgon::Audio
