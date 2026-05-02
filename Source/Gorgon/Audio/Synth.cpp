#include "Synth.h"
#include "Gorgon/Audio/Basic.h"
#include "Gorgon/String.h"
#include "Gorgon/Types.h"
// #include "Gorgon/Utils/Assert.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <mutex>
#include <string>
#include <tuple>
#include <iostream>
#include <utility>
#include <vector>

namespace Gorgon :: Audio {

namespace internal {

    void AppendUInt32LE(std::string &out, uint32_t value) {
        out.push_back(static_cast<char>(value & 0xFF));
        out.push_back(static_cast<char>((value >> 8) & 0xFF));
        out.push_back(static_cast<char>((value >> 16) & 0xFF));
        out.push_back(static_cast<char>((value >> 24) & 0xFF));
    }

    void AppendRiffInfoChunk(std::string &out, const std::string &id, const std::string &value) {
        if(value.empty()) {
            return;
        }

        out += id;

        std::string payload = value;
        payload.push_back('\0');
        if(payload.size() % 2 != 0) {
            payload.push_back('\0');
        }

        AppendUInt32LE(out, static_cast<uint32_t>(payload.size()));
        out += payload;
    }

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


///// METADATA FUNCTIONS /////

bool Synth::MetaData::HasTag(TagType type, const std::string &value) const {
    for(const auto &tag : Tags) {
        if((type == TagType::Any || tag.Type == type) && String::CaseInsensitiveCompare(tag.Value, value) == 0) {
            return true;
        }
    }

    return false;
}

bool Synth::MetaData::HasTags(const std::vector<std::string> &values) const {
    for(const auto& value : values) {
        if(!HasTag(TagType::Any, value)) {
            return false;
        }
    }

    return true;
}

bool Synth::MetaData::HasTags(const std::vector<std::pair<TagType, std::string>> &values) const {
    for(const auto& [type, value] : values) {
        if(!HasTag(type, value)) {
            return false;
        }
    }

    return true;
}

std::pair<std::string, std::string> Synth::MetaData::ToWaveChunk() const {
    std::vector<std::string> genreTags;
    std::vector<std::string> keywordTags;

    for(const auto &tag : Tags) {
        if(tag.Type == Synth::Genre) {
            genreTags.push_back(tag.Value);
        }
        else if(tag.Type == Synth::Custom) {
            keywordTags.push_back(tag.Value);
        }
        else {
            keywordTags.push_back(String::From(tag.Type) + ": " + tag.Value);
        }
    }

    std::string listData = "INFO";
    if(!Title.empty()) {
        internal::AppendRiffInfoChunk(listData, "INAM", Title);
    }
    if(!Artist.empty()) {
        internal::AppendRiffInfoChunk(listData, "IART", Artist);
    }
    if(!Arranger.empty()) {
        internal::AppendRiffInfoChunk(listData, "IENG", Arranger);
    }
    if(!Album.empty()) {
        internal::AppendRiffInfoChunk(listData, "IPRD", Album);
    }
    if(!Copyright.empty()) {
        internal::AppendRiffInfoChunk(listData, "ICOP", Copyright);
    }
    if(!Comment.empty()) {
        internal::AppendRiffInfoChunk(listData, "ICMT", Comment);
    }
    if(!genreTags.empty()) {
        internal::AppendRiffInfoChunk(listData, "IGNR", String::Join(genreTags, ", "));
    }
    if(!keywordTags.empty()) {
        internal::AppendRiffInfoChunk(listData, "IKEY", String::Join(keywordTags, "; "));
    }

    if(listData.size() == 4) {
        return {};
    }

    if(listData.size() % 2 != 0) {
        listData.push_back('\0');
    }

    return {"LIST", std::move(listData)};
}

std::vector<std::pair<std::string, std::string>> Synth::MetaData::ToPairs() const {
    std::vector<std::pair<std::string, std::string>> pairs;

    if(!Title.empty()) {
        pairs.emplace_back("Title", Title);
    }
    if(!Artist.empty()) {
        pairs.emplace_back("Artist", Artist);
    }
    if(!Arranger.empty()) {
        pairs.emplace_back("Arranger", Arranger);
    }
    if(!Album.empty()) {
        pairs.emplace_back("Album", Album);
    }
    if(!Copyright.empty()) {
        pairs.emplace_back("Copyright", Copyright);
    }
    if(!Comment.empty()) {
        pairs.emplace_back("Comment", Comment);
    }

    for(const auto &tag : Tags) {
        if(tag.Type == Synth::Genre) {
            pairs.emplace_back("Genre", tag.Value);
        }
        else {
            pairs.emplace_back(String::From(tag.Type), tag.Value);
        }
    }

    return pairs;
}

///// NODE FUNCTIONS /////

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

Synth::Node Synth::Node::MakeTempo(float tempo, Ramp fade) {
    Node n;

    n.type = Type::Tempo;
    n.tempo.tempo = tempo;
    n.tempo.fade = fade;

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

Synth::Node Synth::Node::MakeVolume(float volume, int channel, Ramp fade) {
    Node n;

    n.type = Type::Volume;
    n.volume.volume = volume;
    n.volume.channel = channel;
    n.volume.fade = fade;

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

Synth::Node Synth::Node::MakeTrackChange(size_t track_index) {
    Node n;

    n.type = Type::TrackChange;
    n.index = track_index;

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
    d.type = WholeNotes;
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
    case WholeNotes:
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
    case WholeNotes:
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
    case WholeNotes:
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
    case WholeNotes:
        return 240.0 / tempo * Units * sample_rate;
    case ClockSeconds:
        return double(Seconds) * sample_rate;
    case NoteFraction:
        return double(notelength) * Units * sample_rate;
    default:
        throw Error(Error::InvalidDuration, "Unsupported duration type");
    }
}

float Synth::Duration::ToNotes(float tempo) const {
    switch(type) {
    case TempoFraction:
        return float(Fraction.Numerator) / float(Fraction.Denominator);
    case WholeNotes:
        return Units;
    case ClockSeconds:
        return Seconds / (240.0f / tempo);
    default:
        throw Error(Error::InvalidDuration, "Unsupported duration type");
    }
}

float Synth::Duration::ToNotes(float tempo, float notelength) const {
    switch(type) {
    case TempoFraction:
        return float(Fraction.Numerator) / float(Fraction.Denominator);
    case WholeNotes:
        return Units;
    case ClockSeconds:
        return Seconds / (240.0f / tempo);
    case NoteFraction:
        return Units * notelength / (240.0f / tempo);
    default:
        throw Error(Error::InvalidDuration, "Unsupported duration type");
    }
}

bool Synth::Duration::IsRelative(const Node &note) const {
    if(type == None)
        throw Error(Error::InvalidDuration, "Cannot determine if duration is relative: " + std::to_string(type));

    if(type == NoteFraction) return note.note.duration.IsRelative();

    return type == TempoFraction || type == WholeNotes;
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

    for(const auto& [k, value] : data) {
        std::string key = String::Replace(k, " ", "");
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
        else if(key == "pitchoffset" || key == "pitch") {
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
    if(note.type != Node::Type::Note) {
        return 0;
    }
    return internal::CalculateOverflow(
        Attack, Decay, Release, state.Separation.Or(Separation), 
        state.Tempo.Current, sample_rate, note
    );
}

namespace {
    template<class T_>
    void SineRenderLoop(
        Containers::Wave &wave, float &t, float tchange, 
        size_t start, size_t end, float volume,
        double &phase, double phasechange, 
        float &env, Synth::TrackState &state,
        T_ getEnv
    ) {
        for(size_t i=start; i<end; i++) {
            env = getEnv(t);

            for(unsigned ch = 0; ch < wave.GetChannelCount(); ch++) {
                float vol;
                if(state.Volume[ch].ChangeRamp.Type != Synth::RampType::None) {
                    state.Volume[ch].CurrentChange += state.Volume[ch].ChangePerSample;
                    vol = state.Volume[ch].ChangeRamp.GetMultiplier(state.Volume[ch].CurrentChange) * (state.Volume[ch].TargetValue - state.Volume[ch].StartValue) + state.Volume[ch].StartValue;
                }
                else {
                    vol = state.Volume[ch].Current;
                }

                float sample = std::sin(float(2.0 * Gorgon::PI * phase));
                wave(i, ch) +=  env * sample * volume * vol;
            }

            phase += phasechange;
            if(phase >= 1.0) phase -= 1.0;
            t += tchange;
        }
    }
}

double Synth::Sine::Render(Containers::Wave &wave, const Node &node, TrackState &state, unsigned int sample_rate) {
    float frequency = NoteToFrequency(node.note.note, state.Octave, PitchOffset);

    double phase = 0.0;
    double phasechange = double(frequency) / sample_rate;

    if(node.type == Node::Type::Rest) return node.note.duration.ToSamples(state.Tempo.Current, sample_rate);
    
    auto [atk, dec, sus, rel] = internal::CalculateADSR(
        Attack, Decay, Release, 
        state.Separation.Or(Separation), state.Tempo.Current,
        sample_rate, 
        node.note.duration.ToSeconds(state.Tempo.Current)
    );

    size_t start = size_t(std::round(state.Sample));
    size_t end = size_t(std::round(state.Sample + atk));

    float tchange = 1.0f / (end - start);
    float t = 0;
    float env = 0.0f;

    SineRenderLoop(wave, t, tchange, start, end, Volume, phase, phasechange, env, state, [&](float t) {
        return Attack.GetMultiplier(t);
    });

    start = end;
    end = size_t(std::round(state.Sample + atk + dec));
    tchange = 1.0f / (end - start);
    t = 0;

    SineRenderLoop(wave, t, tchange, start, end, Volume, phase, phasechange, env, state, [&](float t) {
        return 1.0f - Decay.GetMultiplier(t) * (1.0f - Sustain);
    });

    start = end;
    end = size_t(std::round(state.Sample + atk + dec + sus));
    tchange = 1.0f / (end - start);
    t = 0;

    SineRenderLoop(wave, t, tchange, start, end, Volume, phase, phasechange, env, state, [&](float) {
        return Sustain;
    });

    start = end;
    end = size_t(std::round(state.Sample + atk + dec + sus + rel));
    tchange = 1.0f / (end - start);
    t = 0;
    auto sustain = env; // sustain level at the start of release phase

    SineRenderLoop(wave, t, tchange, start, end, Volume, phase, phasechange, env, state, [&](float t) {
        return sustain - Release.GetMultiplier(t) * sustain;
    });
    
    return node.note.duration.ToSamples(state.Tempo.Current, sample_rate);
}

///// PWM FUNCTIONS /////

void Synth::PWM::LoadSettings(const std::string_view &settings) {
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

    for(const auto& [k, value] : data) {
        std::string key = String::Replace(k, " ", "");
        if(key == "volume") {
            auto [volume, state] = String::FromCLocaleTo<float>(value);
            if(state == String::FromCLocaleToState::Failed) {
                throw Error(Error::InvalidParameter, "Invalid volume value: " + value);
            }
            if(state == String::FromCLocaleToState::ScrapAtTheEnd) {
                throw Error(Error::InvalidParameter, "Extra characters after volume value: " + value);
            }

            Volume = volume / 100.0f;
        }
        else if(key == "rise" || key == "slew") {
            auto [rise, state] = String::FromCLocaleTo<float>(value);
            if(state == String::FromCLocaleToState::Failed) {
                throw Error(Error::InvalidParameter, "Invalid rise/slew time: " + value);
            }
            if(state == String::FromCLocaleToState::ScrapAtTheEnd) {
                throw Error(Error::InvalidParameter, "Extra characters after rise/slew time: " + value);
            }

            Trise = rise;
        }
        else if(key == "dutycycle" || key == "duty") {
            auto [duty, state] = String::FromCLocaleTo<float>(value);
            if(state == String::FromCLocaleToState::Failed) {
                throw Error(Error::InvalidParameter, "Invalid duty cycle value: " + value);
            }
            if(state == String::FromCLocaleToState::ScrapAtTheEnd) {
                throw Error(Error::InvalidParameter, "Extra characters after duty cycle value: " + value);
            }

            DutyCycle = duty;
        }
        else if(key == "separation") {
            Separation = Duration::Parse(value);
        }
        else if(key == "pitchoffset" || key == "pitch") {
            auto [offset, state] = String::FromCLocaleTo<float>(value);
            if(state == String::FromCLocaleToState::Failed) {
                throw Error(Error::InvalidParameter, "Invalid pitch offset value: " + value);
            }
            if(state == String::FromCLocaleToState::ScrapAtTheEnd) {
                throw Error(Error::InvalidParameter, "Extra characters after pitch offset value: " + value);
            }

            PitchOffset = offset;
        }
        else if(key == "resetphase") {
            if(value == "true" || value == "1") {
                ResetPhase = true;
            }
            else if(value == "false" || value == "0") {
                ResetPhase = false;
            }
            else {
                throw Error(Error::InvalidParameter, "Invalid value for reset phase parameter: " + value);
            }
        }
        else {
            throw Error(Error::InvalidParameter, "Unknown parameter for PWM instrument: " + key);
        }
    }
}

double Synth::PWM::Render(Containers::Wave &wave, const Node &node, TrackState &state, unsigned int sample_rate) {
    float frequency = NoteToFrequency(node.note.note, state.Octave, PitchOffset);

    double phasechange = double(frequency) / sample_rate;

    double duration = node.note.duration.ToSamples(state.Tempo.Current, sample_rate);
    double sepduration = state.Separation.Or(Separation).ToSamples(state.Tempo.Current, sample_rate);

    size_t start = size_t(std::round(state.Sample));
    size_t end = size_t(std::round(state.Sample + duration));

    size_t sep = (duration > sepduration) ? (end - size_t(std::round(sepduration))) : start;

    if(ResetPhase) {
        phase = 0.0;
    }

    float base_vol = node.type == Node::Type::Rest ? 0.0f : Volume;
    float alpha = 1.0f;

    if (Trise > 0.0f) {
        float fs = static_cast<float>(sample_rate);
        alpha = 1.0f - std::exp(-2.2f / (fs * Trise));
    }

    for(size_t i=start; i<end; i++) {
        auto target = (phase < DutyCycle) ? 1.0f : -1.0f; 

        if(i == sep) base_vol = 0.0f;

        target *= base_vol;

        current_level += alpha * (target - current_level);

        for(unsigned ch = 0; ch < wave.GetChannelCount(); ch++) {
            float vol;
            if(state.Volume[ch].ChangeRamp.Type != RampType::None) {
                state.Volume[ch].CurrentChange += state.Volume[ch].ChangePerSample;
                vol = state.Volume[ch].ChangeRamp.GetMultiplier(state.Volume[ch].CurrentChange) * (state.Volume[ch].TargetValue - state.Volume[ch].StartValue) + state.Volume[ch].StartValue;
            }
            else {
                vol = state.Volume[ch].Current;
            }

            wave(i, ch) +=  current_level * vol;
        }

        phase += phasechange;
        if(phase >= 1.0) phase -= 1.0;
    }

    return duration;
}

void Synth::PWM::RenderTheEnd(Containers::Wave &wave, TrackState &state, unsigned int sample_rate) {
    if(Trise <= 0.0f || current_level == 0.0f) return;

    float alpha = 1.0f - std::exp(-2.2f / (sample_rate * Trise));

    size_t start = size_t(std::round(state.Sample));
    size_t end   = std::min(size_t(std::round(start + Trise * 2.5 * sample_rate)), wave.GetSize());

    for(size_t i=start; i<end; i++) {
        current_level += alpha * (0.0f - current_level);

        for(unsigned ch = 0; ch < wave.GetChannelCount(); ch++) {
            wave(i, ch) +=  current_level * state.Volume[ch].Current;
        }
    }
}

///// SYNTH FUNCTIONS /////

Synth::Node Synth::ParseNode(const std::string_view& token, int channels) {
    std::string normalized = String::ToLower(String::Trim(std::string{token}));

    if(normalized.empty()) {
        return {};
    }
        
    switch(normalized[0]) {
    case 't': {

        //find : if it exists, it is followed by a duration or a ramp, if it is ramp, it should start with a { and end with a }
        // : must be after tempo value.

        std::string tempo;

        normalized = String::Trim(normalized.substr(1));

        auto columnpos = normalized.find(':');

        Ramp ramp{RampType::None};

        if(columnpos != std::string::npos) {
            tempo = normalized.substr(0, columnpos);
            auto param = String::Trim(normalized.substr(columnpos + 1));

            if(param.empty()) {
                throw Error(Error::InvalidParameter, "Missing parameter after colon in volume token: " + normalized);
            }

            if(param[0] == '{') {
                if(param.back() != '}') {
                    throw Error(Error::InvalidToken, "Mismatched braces in volume ramp parameter: " + param);
                }

                ramp = Ramp::Parse(param.substr(1, param.size() - 2));
            }
            else {
                ramp = Ramp{RampType::Linear, Duration::Parse(param)};
            }
        }
        else {
            tempo = normalized;
        }

        auto [tempoval, res] = String::FromCLocaleTo<float>(tempo);
        if(res == String::FromCLocaleToState::Failed) {
            throw Error(Error::InvalidParameter, "Invalid tempo value: " + tempo);
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw Error(Error::InvalidParameter, "Extra characters after tempo value: " + tempo);
        }
        
        return Node::MakeTempo(tempoval, ramp);
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
        if(normalized[1] == '(') {
            auto endBrace = normalized.find(')');

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

        //find : if it exists, it is followed by a duration or a ramp, if it is ramp, it should start with a { and end with a }
        // : must be after volume value.

        normalized = String::Trim(normalized);

        std::string volume;

        auto columnpos = normalized.find(':');

        Ramp ramp{RampType::None};

        if(columnpos != std::string::npos) {
            volume = normalized.substr(0, columnpos);
            auto param = String::Trim(normalized.substr(columnpos + 1));

            if(param.empty()) {
                throw Error(Error::InvalidParameter, "Missing parameter after colon in volume token: " + normalized);
            }

            if(param[0] == '{') {
                if(param.back() != '}') {
                    throw Error(Error::InvalidToken, "Mismatched braces in volume ramp parameter: " + param);
                }

                ramp = Ramp::Parse(param.substr(1, param.size() - 2));
            }
            else {
                ramp = Ramp{RampType::Linear, Duration::Parse(param)};
            }
        }
        else {
            volume = normalized;
        }
        
        auto [vol, res] = String::FromCLocaleTo<float>(volume.substr(1));

        if(res == String::FromCLocaleToState::Failed) {
            throw Error(Error::InvalidParameter, "Invalid volume value: " + volume.substr(1));
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw Error(Error::InvalidParameter, "Extra characters after volume value: " + volume.substr(1));
        }
        if(vol < 0 || vol > 100) {
            throw Error(Error::InvalidParameter, "Volume must be between 0 and 100: " + std::to_string(vol));
        }

        return Node::MakeVolume(vol / 100.0f, channel, ramp);
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
        else if(normalized[0] >= '0' && normalized[0] <= '9') { //if a number is followed by > , it's a track change.
            if(normalized.back() != '>') {
                throw Error(Error::InvalidToken, "Unrecognized token: " + std::string{token});
            }
            else {
                normalized.pop_back();
            }

            auto [inst, res] = String::FromCLocaleTo<size_t>(normalized);
            if(res == String::FromCLocaleToState::Failed) {
                throw Error(Error::InvalidParameter, "Invalid instrument index: " + normalized);
            }
            if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
                throw Error(Error::InvalidParameter, "Extra characters after instrument index: " + normalized);
            }
            
            return Node::MakeTrackChange(inst);
        }

        throw Error(Error::InvalidToken, "Unrecognized token: " + std::string{token});
    }
}

void Synth::Parse(std::istream &stream) {
    auto guard = std::scoped_lock(critical);

    Reset();

    std::string line;

    size_t track_index = 0;
    Track *track = &tracks[0];

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

                this->channels = std::move(channels);
            }
            else if(var == "artist" || var == "author") {
                metadata.Artist = line;
            }
            else if(var == "title") {
                metadata.Title = line;
            }
            else if(var == "arranger") {
                metadata.Arranger = line;
            }
            else if(var == "album") {
                metadata.Album = line;
            }
            else if(var == "copyright") {
                metadata.Copyright = line;
            }
            else if(var == "comment") {
                if(!metadata.Comment.empty()) {
                    metadata.Comment += "\n";
                }
                metadata.Comment += line;
            }
            else if(var == "genre" ||  var == "mood" || var == "theme" || var == "style" || var == "region" || var == "era" || var == "custom-tag") {
                auto tag = String::Trim(line);
                if(tag.empty()) {
                    throw Error(Error::InvalidParameter, "Tag value cannot be empty");
                }
                TagType type;

                if(var == "genre") type = TagType::Genre;
                else if(var == "mood") type = TagType::Mood;
                else if(var == "theme") type = TagType::Theme;
                else if(var == "style") type = TagType::Style;
                else if(var == "region") type = TagType::Region;
                else if(var == "era") type = TagType::Era;
                else if(var == "custom-tag") type = TagType::Custom;
                else {
                    throw Error(Error::InvalidParameter, "Unrecognized tag type: " + var);
                }

                metadata.Tags.push_back({type, tag});
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

                if(line[0] == '"' || line[0] == '\'') {
                    char quote = line[0];
                    pos = line.find(quote, 1);
                    if(pos == std::string::npos) {
                        throw Error(Error::InvalidParameter, "Mismatched quotes in instrument name for instrument index: " + std::to_string(index));
                    }

                    type = line.substr(1, pos - 1);
                    line = String::Trim(line.substr(pos + 1));
                    pos = 0;
                }
                else {
                    pos = line.find_first_of(" \t(");

                    type = String::ToLower((pos == std::string::npos) ? line : line.substr(0, pos));
                }

                type = String::Replace(type, "_", " ");

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

                if(auto inst = findinstrument(type, instrumentfactories); inst.has_value()) {
                    factory = inst.value();
                }
                else if(auto inst = findinstrument(type, baseinstrumentfactories); inst.has_value()) {
                    factory = inst.value();
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

            // if this is a note or rest node and we are still on track 0, move to track 1 as track 0 is reserved for global settings
            if(track_index == 0 && (node.type == Node::Type::Note || node.type == Node::Type::Rest)) {
                track_index = 1;
                tracks.push_back(Track{track_index});
                track = &tracks.back();
                track->Nodes = tracks[0].Nodes; // copy global settings to the new track
            }
            else if(node.type == Node::Type::TrackChange) {
                track_index = node.index;
                if(track_index == tracks.size()) {
                    tracks.push_back(Track{track_index});
                    tracks.back().Nodes = tracks[0].Nodes; // copy global settings to the new track
                }
                else if(track_index > tracks.size()) {
                    throw Error(Error::InvalidParameter, "Track index cannot have gaps: " + std::to_string(track_index));
                }

                track = &tracks[track_index];

                continue; // track change node is not needed to be stored in track data
            }

            if(node.type != Node::Type::NoOp) {
                track->Nodes.push_back(node);
            }
        }
    }
}

Synth::AudioDuration Synth::CalculateSamples(unsigned int sample_rate) const {
    auto guard = std::scoped_lock(critical);

    auto [total, end] = calculatesamples(sample_rate);
    return {static_cast<size_t>(std::ceil(total)), static_cast<size_t>(std::ceil(end))};
}

std::pair<double, double> Synth::calculatesamples(unsigned int sample_rate) const {
    double total_data = 0, music_end = 0;

    for(const auto &track : tracks) {
        TrackState state;

        double total = 0;

        for(const auto& node : track.Nodes) {
            switch(node.type) {
            case Node::Type::Rest:
            case Node::Type::Note: {
                double duration = node.note.duration.ToSamples(state.Tempo.Current, sample_rate);
                double overflow = 0;

                // If tempo has a ramp, we need to calculate the duration of the note from the center of the note as it might 
                // effect the duration of the note
                if(state.Tempo.ChangeRamp.Type != RampType::None) {
                    double tempodur;

                    auto fn = [&](auto &dur, auto tempo) { return state.Tempo.ChangeRamp.Span.IsRelative() ? dur.ToNotes(tempo) : dur.ToSeconds(tempo); };
                    
                    //if relativity of note and tempo is same. Duration is not used in the calculation, therefore, it results
                    //in a perfect value.
                    if(node.note.duration.IsRelative() == state.Tempo.ChangeRamp.Span.IsRelative()) {
                        tempodur = fn(node.note.duration, state.Tempo.Current);
                    }
                    else {
                        //otherwise, we need to estimate the calculation
                        double start_duration = fn(node.note.duration, state.Tempo.Current);
                        auto change = state.Tempo.ChangePerTime * start_duration;
                        auto tempo = state.Tempo.ChangeRamp.GetMultiplier(change) * (state.Tempo.TargetValue - state.Tempo.StartValue) + state.Tempo.StartValue;
                        double end_duration = fn(node.note.duration, tempo);

                        tempodur = (start_duration + end_duration) / 2;
                    }

                    auto change = state.Tempo.CurrentChange + state.Tempo.ChangePerTime * tempodur / 2;
                    if(change >= 0.9999999) {
                        change = 1;
                    }

                    state.Tempo.Current = state.Tempo.ChangeRamp.GetMultiplier(change) * (state.Tempo.TargetValue - state.Tempo.StartValue) + state.Tempo.StartValue;
                }

                if(state.InstrumentIndex != 0) {
                    auto &instr = instruments[long(state.InstrumentIndex) - 1];

                    duration = instr.CalculateSamples(node, state, sample_rate);
                    overflow = instr.ReleaseOverflow(state, sample_rate, node);
                }

                //now we need to calculate actual change in tempo for the duration of the note and apply it to the state.
                if(state.Tempo.ChangeRamp.Type != RampType::None) {
                    //we cannot rely on the note duration as instrument might modify it.
                    if(state.Tempo.ChangeRamp.Span.IsRelative()) {
                        auto relative_duration = (duration / sample_rate) / (240 / state.Tempo.Current);

                        state.Tempo.CurrentChange += state.Tempo.ChangePerTime * relative_duration;
                    }
                    else {
                        auto absolue_duration = (duration / sample_rate);

                        state.Tempo.CurrentChange += state.Tempo.ChangePerTime * absolue_duration;
                    }

                    if(state.Tempo.CurrentChange >= 0.9999999) {
                        state.Tempo.CurrentChange = 1.0;
                        state.Tempo.Current = state.Tempo.TargetValue;
                        state.Tempo.ChangeRamp = {RampType::None};
                    }
                    else {
                        state.Tempo.Current = state.Tempo.ChangeRamp.GetMultiplier(state.Tempo.CurrentChange) * (state.Tempo.TargetValue - state.Tempo.StartValue) + state.Tempo.StartValue;
                    }
                }

                state.Sample += duration;
                total = std::max(total, state.Sample + overflow);
                break;
            }
            case Node::Type::Tempo:{
                auto tempo = node.tempo;
                state.Tempo.StartValue = state.Tempo.Current;
                state.Tempo.TargetValue = tempo.tempo;
                state.Tempo.CurrentChange = 0;

                if(tempo.fade.Type != RampType::None) {
                    state.Tempo.ChangeRamp = tempo.fade;
                    
                    if(tempo.fade.Span.IsRelative()) {
                        state.Tempo.ChangePerTime = 1.0 / tempo.fade.Span.ToNotes(state.Tempo.Current);
                    }
                    else {
                        state.Tempo.ChangePerTime = 1.0 / tempo.fade.Span.ToSeconds(state.Tempo.Current);
                    }
                }
                else {
                    state.Tempo.Current = tempo.tempo;
                    state.Tempo.ChangeRamp = Ramp{RampType::None};
                }

                break;
            }
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

        if(total > total_data) {
            total_data = total;
            music_end = state.Sample;
        }
        if(state.Sample > music_end) {
            music_end = state.Sample;
        }
    }

    return {total_data, music_end};
}

float Synth::CalculateDuration() const { 
    return CalculateSamples(151200).Total / 151200.0f; 
}

Containers::Wave Synth::Render(unsigned int sample_rate) const {
    auto guard = std::scoped_lock(critical);

    Containers::Wave wave(size_t(std::ceil(calculatesamples(sample_rate).first)), unsigned(sample_rate), channels);
    wave.Clear();

    // track 0 is reserved for global settings and is not rendered, we start from track 1
    for(size_t i=1; i<tracks.size(); i++) {
        const auto &track = tracks[i];

        TrackState state;
        state.Volume = std::vector(channels.size(), TrackState::TrackedValue{1.0f});

        for(const auto &node: track.Nodes) {
            switch(node.type) {
            case Node::Type::Tempo: {
                auto tempo = node.tempo;
                state.Tempo.StartValue = state.Tempo.Current;
                state.Tempo.TargetValue = tempo.tempo;
                state.Tempo.CurrentChange = 0;

                if(tempo.fade.Type != RampType::None) {
                    state.Tempo.ChangeRamp = tempo.fade;
                    
                    if(tempo.fade.Span.IsRelative()) {
                        state.Tempo.ChangePerTime = 1.0 / tempo.fade.Span.ToNotes(state.Tempo.Current);
                    }
                    else {
                        state.Tempo.ChangePerTime = 1.0 / tempo.fade.Span.ToSeconds(state.Tempo.Current);
                    }
                }
                else {
                    state.Tempo.Current = tempo.tempo;
                    state.Tempo.ChangeRamp = Ramp{RampType::None};
                }

                break;
            }
            case Node::Type::OctaveAbsolute:
                state.Octave = node.octave;
                break;

            case Node::Type::OctaveRelative:
                state.Octave += node.octave;
                break;

            case Node::Type::Volume: {
                if(node.volume.fade.Type == RampType::None) {
                    if(node.volume.channel == 0) {
                        std::fill(state.Volume.begin(), state.Volume.end(), TrackState::TrackedValue{node.volume.volume});
                    }
                    else {
                        state.Volume[node.volume.channel - 1] = {node.volume.volume};
                    }
                }
                else {
                    auto set_channel = [&](TrackState::TrackedValue &v) {
                        auto vol = node.volume;

                        v.StartValue = v.Current;
                        v.TargetValue = vol.volume;
                        v.CurrentChange = 0;

                        if(vol.fade.Type != RampType::None) {
                            v.ChangeRamp = vol.fade;
                            
                            if(vol.fade.Span.IsRelative()) {
                                v.ChangePerTime = 1.0 / vol.fade.Span.ToNotes(state.Tempo.Current);
                            }
                            else {
                                v.ChangePerTime = 1.0 / vol.fade.Span.ToSeconds(state.Tempo.Current);
                            }
                        }
                        else {
                            v.Current = vol.volume;
                        }
                    };

                    if(node.volume.channel == 0) {
                        std::for_each(state.Volume.begin(), state.Volume.end(), set_channel);
                    }
                    else {
                        set_channel(state.Volume[node.volume.channel - 1]);
                    }
                }

                break;
            }
            case Node::Type::Separation:
                state.Separation = node.duration;
                break;

            case Node::Type::InstrumentChange:
                if(state.InstrumentIndex > 0) {
                    instruments[long(state.InstrumentIndex) - 1].RenderTheEnd(wave, state, sample_rate);
                }

                state.InstrumentIndex = node.index;

                if(state.InstrumentIndex > size_t(instruments.GetSize())) {
                    throw Error(Error::InvalidParameter, "Instrument index out of range: " + std::to_string(state.InstrumentIndex));
                }
                
                break;

            case Node::Type::Rest:
            case Node::Type::Note: {
                double duration = node.note.duration.ToSamples(state.Tempo.Current, sample_rate);

                // If tempo has a ramp, we need to calculate the duration of the note from the center of the note as it might 
                // effect the duration of the note
                if(state.Tempo.ChangeRamp.Type != RampType::None) {
                    double tempodur;

                    auto fn = [&](auto &dur, auto tempo) { return state.Tempo.ChangeRamp.Span.IsRelative() ? dur.ToNotes(tempo) : dur.ToSeconds(tempo); };
                    
                    //if relativity of note and tempo is same. Duration is not used in the calculation, therefore, it results
                    //in a perfect value.
                    if(node.note.duration.IsRelative() == state.Tempo.ChangeRamp.Span.IsRelative()) {
                        tempodur = fn(node.note.duration, state.Tempo.Current);
                    }
                    else {
                        //otherwise, we need to estimate the calculation
                        double start_duration = fn(node.note.duration, state.Tempo.Current);
                        auto change = state.Tempo.ChangePerTime * start_duration;
                        auto tempo = state.Tempo.ChangeRamp.GetMultiplier(change) * (state.Tempo.TargetValue - state.Tempo.StartValue) + state.Tempo.StartValue;
                        double end_duration = fn(node.note.duration, tempo);

                        tempodur = (start_duration + end_duration) / 2;
                    }

                    auto change = state.Tempo.CurrentChange + state.Tempo.ChangePerTime * tempodur / 2;
                    if(change >= 0.9999999) {
                        change = 1;
                    }

                    state.Tempo.Current = state.Tempo.ChangeRamp.GetMultiplier(change) * (state.Tempo.TargetValue - state.Tempo.StartValue) + state.Tempo.StartValue;
                }

                //calculate duration again after tempo update
                if(state.InstrumentIndex != 0) {
                    duration = instruments[long(state.InstrumentIndex) - 1].CalculateSamples(node, state, sample_rate);
                }
                else {
                    duration = node.note.duration.ToSamples(state.Tempo.Current, sample_rate);
                }

                // volume change is applied by the instrument during the note rendering. We just need to calculate per sample
                // sample change here. However, if instrument wants to change the duration of the note, it needs to readjust
                // this value.
                std::vector<double> finalvols(state.Volume.size());
                for(size_t i=0; i<state.Volume.size(); i++) {
                    auto &vol = state.Volume[i];
                    if(vol.ChangeRamp.Type != RampType::None) {
                        if(vol.ChangeRamp.Span.IsRelative()) {
                            vol.ChangePerSample = vol.ChangePerTime / ((240 / state.Tempo.Current) * sample_rate);
                        }
                        else {
                            vol.ChangePerSample = vol.ChangePerTime / sample_rate;
                        }
                        
                        finalvols[i] = vol.CurrentChange + vol.ChangePerSample * duration;
                        if(finalvols[i] > 1.0) finalvols[i] = 1.0;
                    }
                }

                // Instrument index 0 is silent, we just advance the sample position without rendering anything
                if(state.InstrumentIndex != 0) {
                    instruments[long(state.InstrumentIndex) - 1].Render(wave, node, state, sample_rate);
                }

                //now we need to calculate actual change in tempo for the duration of the note and apply it to the state.
                if(state.Tempo.ChangeRamp.Type != RampType::None) {
                    //we cannot rely on the note duration as instrument might modify it.
                    if(state.Tempo.ChangeRamp.Span.IsRelative()) {
                        auto relative_duration = (duration / sample_rate) / (240 / state.Tempo.Current);

                        state.Tempo.CurrentChange += state.Tempo.ChangePerTime * relative_duration;
                    }
                    else {
                        auto absolue_duration = (duration / sample_rate);

                        state.Tempo.CurrentChange += state.Tempo.ChangePerTime * absolue_duration;
                    }

                    if(state.Tempo.CurrentChange >= 0.9999999) {
                        state.Tempo.CurrentChange = 1.0;
                        state.Tempo.Current = state.Tempo.TargetValue;
                        state.Tempo.ChangeRamp = {RampType::None};
                    }
                    else {
                        state.Tempo.Current = state.Tempo.ChangeRamp.GetMultiplier(state.Tempo.CurrentChange) * (state.Tempo.TargetValue - state.Tempo.StartValue) + state.Tempo.StartValue;
                    }
                }

                //apply volume changes for the duration of the note
                for(size_t i=0; i<state.Volume.size(); i++) {
                    auto &vol = state.Volume[i];

                    if(vol.ChangeRamp.Type != RampType::None) {
                        vol.CurrentChange = finalvols[i];
                        
                        if(vol.CurrentChange >= 0.9999999) {
                            vol.CurrentChange = 1.0;
                            vol.Current = vol.TargetValue;
                            vol.ChangeRamp = {RampType::None};
                        }
                        else {
                            vol.Current = vol.ChangeRamp.GetMultiplier(vol.CurrentChange) * (vol.TargetValue - vol.StartValue) + vol.StartValue;
                        }
                    }
                }

                state.Sample += duration;
                break;
            }

            case Node::Type::NoOp:
                break;

            case Node::Type::TrackChange:
                throw Error(Error::InvalidToken, "Track change cannot exists in track data, it should have been processed during parsing: " + std::to_string(node.index));
            }
        }

        if(state.InstrumentIndex > 0) {
            instruments[long(state.InstrumentIndex) - 1].RenderTheEnd(wave, state, sample_rate);
        }
    }

    return wave;
}

void Synth::AddInstrumentFactory(std::function<Instrument &()> factory, const std::string &name) {
    auto guard = std::scoped_lock(critical);

    if(name.empty()) {
        throw Error(Error::InvalidParameter, "Instrument factory name cannot be empty");
    }
    if(!factory) {
        throw Error(Error::InvalidParameter, "Instrument factory cannot be empty");
    }

    auto current = findinstrumentindex(name, instrumentfactories);
    if(current.has_value()) {
        instrumentfactories[current.value()] = {name, factory};
    }
    else {
        instrumentfactories.push_back({String::ToLower(name), factory});
    }
}

void Synth::RemoveInstrumentFactory(const std::string &name) {
  auto guard = std::scoped_lock(critical);

    auto index = findinstrumentindex(name, instrumentfactories);
    if(index.has_value()) {
        instrumentfactories.erase(instrumentfactories.begin() + index.value());
    }
}

void Synth::ClearInstrumentFactories() {
  auto guard = std::scoped_lock(critical);

  instrumentfactories.clear();
  instrumentfactories.push_back({"sine", []() -> Instrument & { return *new Sine(); }});
}

size_t Synth::AddInstrument(Instrument& instrument) {
    auto guard = std::scoped_lock(critical);
    
    instruments.Add(instrument);
    return size_t(instruments.GetSize());
}

void Synth::SetInstrument(size_t index, Instrument& instrument) {
    auto guard = std::scoped_lock(critical);

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
    auto guard = std::scoped_lock(critical);

    if(index == 0 || index > size_t(instruments.GetSize())) {
        throw Error(Error::InvalidParameter, "Instrument index out of range: " + std::to_string(index));
    }
    instruments.Delete(long(index - 1));
}

void Synth::AddChannel(Audio::Channel channel) {
    auto guard = std::scoped_lock(critical);

    if(HasChannel(channel)) return;

    channels.push_back(channel);
    std::sort(channels.begin(), channels.end(), [](Audio::Channel a, Audio::Channel b) {
        return static_cast<int>(a) < static_cast<int>(b);
    });
}

bool Synth::HasChannel(Audio::Channel channel) const {
    return std::find(channels.begin(), channels.end(), channel) != channels.end();
}

void Synth::RemoveChannel(Audio::Channel channel) {
    auto guard = std::scoped_lock(critical);

    auto it = std::find(channels.begin(), channels.end(), channel);
    if(it != channels.end()) {
        channels.erase(it);
    }
}

void Synth::AddNode(Node node, size_t track) {
    auto guard = std::scoped_lock(critical);

    if(track == tracks.size()) {
        tracks.resize(track + 1);
    }
    else if(track > tracks.size()) {
        throw Error(Error::InvalidParameter, "Track index cannot have gaps: " + std::to_string(track));
    }

    if(node.type == Node::Type::TrackChange) {
        throw Error(Error::InvalidToken, "Track change node cannot be added to track data");
    }

    tracks[track].Nodes.push_back(node);
}

void Synth::RemoveNode(size_t index, size_t track) {
    auto guard = std::scoped_lock(critical);

    tracks[track].Nodes.erase(tracks[track].Nodes.begin() + index);
}

std::vector<std::string> Synth::GetInstrumentRegistry() {
    std::vector<std::string> names;

    for(const auto& [name, factory] : baseinstrumentfactories) {
        names.push_back(name);
    }

    return names;
}

const std::vector<Synth::FactoryEntry> Synth::baseinstrumentfactories = {
    //BASE INSTRUMENTS
    {"sine", []() -> Instrument& { return *new Sine(); }},
    {"pwm", []() -> Instrument& { return *new PWM(); }},

    //SINE BASED INSTRUMENTS
    {"synth", []() -> Instrument& { 
        static Sine sine;
        sine.Name = "Synth";
        sine.Attack = {RampType::Linear, Synth::Duration::FromFraction(128)};
        sine.Decay = {RampType::Linear, Synth::Duration::FromFraction(16)};
        sine.Sustain = 0.8f;
        sine.Release = {RampType::Linear, Synth::Duration::FromFraction(16)};
        sine.Description = "A classic synth sound with a quick attack and moderate decay, perfect for leads and pads.";
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
        sine.Description = "A soft and airy flute sound with a gentle attack and long release, ideal for melodic lines and atmospheric textures.";
        return sine.Clone(); 
    }},
    {"chiptune", []() -> Instrument& { 
        static Sine sine;
        sine.Name = "Chiptune";
        sine.Attack = {RampType::None};
        sine.Decay = {RampType::Linear, Synth::Duration::FromFraction(16)};
        sine.Sustain = 0.0f;
        sine.Release = {RampType::Linear, Synth::Duration::FromFraction(16)};
        sine.Volume = 1.0f;
        sine.Description = "A classic chiptune sound with a quick attack and short decay, ideal for retro game music.";
        return sine.Clone(); 
    }},
    {"ambient pad", []() -> Instrument& { 
        static Sine sine;
        sine.Name = "Ambient Pad";
        sine.Attack = {RampType::Linear, Synth::Duration::FromFraction(2)};
        sine.Decay = {RampType::Linear, Synth::Duration::FromFraction(4)};
        sine.Sustain = 0.8f;
        sine.Release = {RampType::Linear, Synth::Duration::FromFraction(1)};
        sine.Volume = 0.4f;
        sine.Description = "A soft and evolving ambient pad sound with a slow attack and long release, perfect for atmospheric textures.";
        return sine.Clone(); 
    }},
    {"electric piano", []() -> Instrument& { 
        static Sine sine;
        sine.Name = "Electric Piano";
        sine.Attack = {RampType::SCurve, Synth::Duration::FromSeconds(0.02f)};
        sine.Decay = {RampType::SCurve, Synth::Duration::FromSeconds(0.3f)};
        sine.Sustain = 0.3f;
        sine.Release = {RampType::SCurve, Synth::Duration::FromSeconds(0.6f)};
        sine.Description = "A classic electric piano sound with a quick attack and moderate decay, ideal for chords and melodic lines.";
        return sine.Clone(); 
    }},
    {"deep sub bass", []() -> Instrument& { 
        static Sine sine;
        sine.Name = "Deep Sub Bass";
        sine.Attack = {RampType::Linear, Synth::Duration::FromSeconds(0.01f)};
        sine.Decay = {RampType::None};
        sine.Sustain = 1.0f;
        sine.Release = {RampType::SCurve, Synth::Duration::FromSeconds(0.15f)};
        sine.PitchOffset = -24.0f; // 2 octaves down
        sine.Volume = 0.7f;
        sine.Description = "A deep sub bass sound with a quick attack and long sustain, ideal for low-end support.";
        return sine.Clone(); 
    }},
    {"xylophone", []() -> Instrument& { 
        static Sine sine;
        sine.Name = "Xylophone";
        sine.Attack = {RampType::SCurve, Synth::Duration::FromSeconds(0.01f)};
        sine.Decay = {RampType::SCurve, Synth::Duration::FromSeconds(0.6f)};
        sine.Sustain = 0.3f;
        sine.Release = {RampType::SCurve, Synth::Duration::FromSeconds(1.2f)};
        sine.Volume = 0.6f;
        sine.PitchOffset = 12.0f; // 1 octave up
        sine.Description = "A bright and percussive xylophone sound with a quick attack and moderate decay, perfect for melodic lines and rhythmic patterns.";
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
        sine.Description = "A bright and resonant bell sound with a quick attack and long decay, ideal for melodic accents and atmospheric effects.";
        return sine.Clone(); 
    }},
    //PWM BASED INSTRUMENTS
    {"square lead", []() -> Instrument& { 
        static PWM inst;
        inst.Name = "Square Lead";
        inst.DutyCycle = 0.5f;
        inst.Trise = 0.0002f; // Slight 0.2ms slew for headache prevention
        inst.Volume = 0.8f;
        inst.Separation = Synth::Duration::FromFraction(32);
        inst.PitchOffset = 0.0f;
        inst.ResetPhase = false; // Smooth legato transitions
        inst.Description = "A pure 50% duty cycle square wave. The definitive 8-bit lead synth for main melodies.";
        return inst.Clone(); 
    }},
    {"thin pulse", []() -> Instrument& { 
        static PWM inst;
        inst.Name = "Thin Pulse";
        inst.DutyCycle = 0.125f; // 12.5% duty cycle creates a bright, thin buzz
        inst.Trise = 0.0001f; // Keep it sharp
        inst.Volume = 0.6f;   // Lower volume because high frequencies pierce easily
        inst.Separation = Synth::Duration::FromFraction(32);
        inst.PitchOffset = 0.0f;
        inst.ResetPhase = false;
        inst.Description = "A sharp 12.5% duty cycle pulse wave. Buzzy and bright, perfect for rapid arpeggios and harpsichord-like textures.";
        return inst.Clone(); 
    }},
    {"punchy bass", []() -> Instrument& { 
        static PWM inst;
        inst.Name = "Punchy Bass";
        inst.DutyCycle = 0.25f; // 25% gives a slightly woody, hollow bass tone
        inst.Trise = 0.0005f; // Slightly more slew to keep the low end warm
        inst.Volume = 1.0f;
        inst.Separation = Synth::Duration::FromFraction(16); // Wider gap for staccato punch
        inst.PitchOffset = -12.0f; // Drop it a full octave
        inst.ResetPhase = true; // ESSENTIAL: Guarantees a hard, identical transient on every note
        inst.Description = "A 25% pulse wave dropped an octave with forced phase reset. Delivers tight, uniform percussive low-end.";
        return inst.Clone(); 
    }},
    {"soft triangle", []() -> Instrument& { 
        static PWM inst;
        inst.Name = "Soft Triangle";
        inst.DutyCycle = 0.5f;
        inst.Trise = 0.008f; // Heavy slew limiting turns the square into a triangle
        inst.Volume = 1.0f;
        inst.Separation = Synth::Duration::FromFraction(64); // Minimal gap for maximum glide
        inst.PitchOffset = 0.0f;
        inst.ResetPhase = false;
        inst.Description = "A 50% square wave with heavy slew limiting (8ms) to roll off harsh harmonics, creating a warm, triangle-like tone for pads and soft leads.";
        return inst.Clone(); 
    }},
};

} // namespace Gorgon::Audio
