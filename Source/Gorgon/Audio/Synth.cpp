#include "Synth.h"
#include "Gorgon/Audio/Basic.h"
#include "Gorgon/String.h"
#include "Gorgon/Utils/Assert.h"
#include <tuple>
#include <iostream>

namespace Gorgon :: Audio {

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

Synth::Duration Synth::Duration::FromFraction(int numerator, int denominator) {
  Duration d;
  d.type = Fraction;
  d.fraction.numerator = numerator;
  d.fraction.denominator = denominator;
  return d;
}

Synth::Duration Synth::Duration::FromUnits(float units) {
    Duration d;
    d.type = Units;
    d.units = units;
    return d;
}

Synth::Duration Synth::Duration::FromSeconds(float seconds) {
    Duration d;
    d.type = Seconds;
    d.seconds = seconds;
    return d;
}

Synth::Node Synth::ParseNode(const std::string_view& token, int channels) {
    std::string normalized = String::ToLower(String::Trim(std::string{token}));

    if(normalized.empty()) {
        return {};
    }
        
    switch(normalized[0]) {
    case 't': {
        auto [tempo, res] = String::FromCLocaleTo<float>(normalized.substr(1));
        if(res == String::FromCLocaleToState::Failed) {
            throw ParseError(ParseError::InvalidParameter, "Invalid tempo value: " + normalized.substr(1));
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw ParseError(ParseError::InvalidParameter, "Extra characters after tempo value: " + normalized.substr(1));
        }
        return Node::MakeTempo(tempo);
    }
    case 'o': {
        auto [oct, res] = String::FromCLocaleTo<int>(normalized.substr(1));
        if(res == String::FromCLocaleToState::Failed) {
            throw ParseError(ParseError::InvalidParameter, "Invalid octave value: " + normalized.substr(1));
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw ParseError(ParseError::InvalidParameter, "Extra characters after octave value: " + normalized.substr(1));
        }
        return Node::MakeOctaveAbsolute(oct);
    }
    case '>':
        return Node::MakeOctaveRelative(1);
    case '<':
        return Node::MakeOctaveRelative(-1);
    case 'v': {
        int channel = 0;
        if(normalized[1] == '{') {
            auto endBrace = normalized.find('}');

            if(endBrace == std::string::npos) {
                throw ParseError(ParseError::InvalidParameter, "Missing closing brace for volume channel: " + normalized);
            }

            String::FromCLocaleToState res;
            auto channelstr = normalized.substr(2, endBrace - 2);
            std::tie(channel, res) = String::FromCLocaleTo<int>(channelstr);

            if(res == String::FromCLocaleToState::Failed) {
                throw ParseError(ParseError::InvalidParameter, "Invalid volume channel: " + channelstr);
            }
            if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
                throw ParseError(ParseError::InvalidParameter, "Extra characters after volume channel: " + channelstr);
            }

            if(channel < 0) {
                throw ParseError(ParseError::InvalidParameter, "Volume channel cannot be negative: " + std::to_string(channel));
            }
            else if(channel > channels) {
                throw ParseError(ParseError::InvalidParameter, "Volume channel index out of range: " + std::to_string(channel));
            }

            normalized.erase(1, endBrace);
        }
        
        auto [vol, res] = String::FromCLocaleTo<float>(normalized.substr(1));

        if(res == String::FromCLocaleToState::Failed) {
            throw ParseError(ParseError::InvalidParameter, "Invalid volume value: " + normalized.substr(1));
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw ParseError(ParseError::InvalidParameter, "Extra characters after volume value: " + normalized.substr(1));
        }
        if(vol < 0 || vol > 100) {
            throw ParseError(ParseError::InvalidParameter, "Volume must be between 0 and 100: " + std::to_string(vol));
        }

        return Node::MakeVolume(vol / 100.0f, channel);
    }

    case 'r':
        return Node::MakeRest(Duration::Parse(normalized.substr(1)));

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
                throw ParseError(ParseError::InvalidToken, "Invalid note: " + std::string{normalized[0]});
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
                        throw ParseError(ParseError::InvalidParameter, "Cannot apply sharp to note B: " + std::string{token});
                    }
                    note = static_cast<Note>((static_cast<int>(note) + 1) % 12);
                    off = 2;
                }
                else if(normalized[1] == '-') {
                    if(note == Note::C) {
                        throw ParseError(ParseError::InvalidParameter, "Cannot apply flat to note C: " + std::string{token});
                    }
                    note = static_cast<Note>((static_cast<int>(note) + 11) % 12);
                    off = 2;
                }
            }

            return Node::MakeNote(note, Duration::Parse(normalized.substr(off)), slide);
        }

        throw ParseError(ParseError::InvalidToken, "Unrecognized token: " + std::string{token});
    }
}

void Synth::Parse(std::istream &stream) {
    Nodes.clear();

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
                    throw ParseError(ParseError::InvalidParameter, "Channels variable cannot be empty");
                }

                if(line[0] == '[') {
                    if(line.back() != ']') {
                        throw ParseError(ParseError::InvalidParameter, "Mismatched brackets in channels variable: " + line);
                    }

                    line.pop_back();
                    line.erase(0, 1);

                    while(!line.empty()) {
                        ch = String::Trim(String::Extract(line, ','));

                        if(ch.empty()) {
                            throw ParseError(ParseError::InvalidParameter, "Empty channel name in channels variable");
                        }

                        Audio::Channel channel = Channel::Unknown;
                        channel = String::To<Audio::Channel>(ch);

                        if(channel == Channel::Unknown) {
                            throw ParseError(ParseError::InvalidParameter, "Unrecognized channel name: " + ch);
                        }

                        channels.push_back(channel);
                    }
                }
                else {
                    auto [channelcount, state] = String::FromCLocaleTo<int>(line);

                    if(state == String::FromCLocaleToState::Failed) {
                        throw ParseError(ParseError::InvalidParameter, "Invalid channel count: " + line);
                    }
                    if(state == String::FromCLocaleToState::ScrapAtTheEnd) {
                        throw ParseError(ParseError::InvalidParameter, "Extra characters after channel count: " + line);
                    }
                    if(channelcount < 1 || channelcount > 6) {
                        throw ParseError(ParseError::InvalidParameter, "Channel count must be between 1 and 6: " + std::to_string(channelcount));
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
                throw ParseError(ParseError::InvalidParameter, "Unrecognized variable: " + var);
            }

            continue;
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

            if(node.type != Node::Type::NoOp) {
                Nodes.push_back(node);
            }
        }
    }
}

size_t Synth::CalculateSamples(float sample_rate) const {
    double samples = 0;
    float tempo = 120.0f; 

    for(const auto& node : Nodes) {
        switch(node.type) {
        case Node::Type::Note:
        case Node::Type::Rest:
            samples += node.note.duration.ToSamples(tempo, sample_rate);
            break;
        case Node::Type::Tempo:
            tempo = node.tempo;
            break;
        default:
            break;
        }
    }

    return size_t(samples);
}

float Synth::CalculateDuration() const { 
    return CalculateSamples(151200) / 151200.0f; 
}

Containers::Wave Synth::Render(float sample_rate) const {
    Containers::Wave wave(CalculateSamples(sample_rate), sample_rate, Channels);

    if(Channels != std::vector<Audio::Channel>{Audio::Channel::Mono}) {
        Utils::NotImplemented("Only mono output is supported currently");
    }

    TrackState state;
    state.Volume = std::vector<float>(Channels.size(), 1.0f);

    for(const auto &node: Nodes) {
        switch(node.type) {
        case Node::Type::Rest:
            state.Sample += node.note.duration.ToSamples(state.Tempo, sample_rate);
            break;

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
        
        case Node::Type::Note: {
            float frequency = NoteToFrequency(node.note.note, state.Octave);
            size_t duration = node.note.duration.ToSamples(state.Tempo, sample_rate);

            size_t notesep = std::min(size_t(duration / 10), Duration::FromFraction(16).ToSamples(state.Tempo, sample_rate));
            duration -= notesep;

            for(size_t i = 0; i < duration; i++) {
                float fade = 1.0f;
                if(i < notesep) {
                    fade = float(i) / notesep;
                }
                else if(i > duration - notesep) {
                    fade = float(duration - i) / notesep;
                }

                float sample = std::sin(2.0f * float(M_PI) * frequency * (state.Sample + i) / sample_rate);

                for(size_t ch = 0; ch < Channels.size(); ch++) {
                    wave(state.Sample + i, ch) = sample * state.Volume[ch] * fade;
                }
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

Synth::Duration Synth::Duration::Parse(const std::string_view& token) {
    if(token.empty()) return Duration::FromFraction(4);

    std::string normalized = String::ToLower(String::Trim(std::string{token}));

    if(normalized.find('/') != std::string::npos) {
        auto nominator = String::Extract(normalized, '/');
        if(nominator.empty()) {
            throw ParseError(ParseError::InvalidParameter, "Missing numerator in duration fraction: " + normalized);
        }
        if(normalized.empty()) {
            throw ParseError(ParseError::InvalidParameter, "Missing denominator in duration fraction: " + normalized);
        }
        int den;
        auto [nom, res] = String::FromCLocaleTo<int>(nominator);
        if(res == String::FromCLocaleToState::Failed) {
            throw ParseError(ParseError::InvalidParameter, "Invalid duration value: " + normalized);
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw ParseError(ParseError::InvalidParameter, "Extra characters after duration value: " + normalized);
        }
        std::tie(den, res) = String::FromCLocaleTo<int>(normalized);
        if(res == String::FromCLocaleToState::Failed) {
            throw ParseError(ParseError::InvalidParameter, "Invalid duration value: " + normalized);
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw ParseError(ParseError::InvalidParameter, "Extra characters after duration value: " + normalized);
        }
        return Duration::FromFraction(nom, den);
    }
    
    if(normalized[0] == '(') {
        if(normalized.back() != ')') {
            throw ParseError(ParseError::InvalidToken, "Mismatched parentheses in duration token: " + std::string{token});
        }

        auto [seconds, res] = String::FromCLocaleTo<float>(normalized.substr(1, normalized.size() - 2));
        if(res == String::FromCLocaleToState::Failed) {
            throw ParseError(ParseError::InvalidParameter, "Invalid duration value: " + normalized);
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {   
            throw ParseError(ParseError::InvalidParameter, "Extra characters after duration value: " + normalized);
        }

        return Duration::FromSeconds(seconds);
    }

    // this last state is 3 in 1. If ends with dot, it's a fraction with 1.5 multiplier.
    // if it has a dot in the middle, it's unit based duration. Otherwise, it's a simple fraction.
    if(auto pos = normalized.find('.'); pos != std::string::npos) {
        if(pos == normalized.size() - 1) {
            auto [base, res] = String::FromCLocaleTo<int>(normalized.substr(0, pos));
            if(res == String::FromCLocaleToState::Failed) {
                throw ParseError(ParseError::InvalidParameter, "Invalid duration value: " + normalized.substr(0, pos));
            }
            if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
                throw ParseError(ParseError::InvalidParameter, "Extra characters after duration value: " + normalized.substr(0, pos));
            }
            return Duration::FromFraction(3, base * 2);
        }
        else {
            auto [units, res] = String::FromCLocaleTo<float>(normalized);
            if(res == String::FromCLocaleToState::Failed) {
                throw ParseError(ParseError::InvalidParameter, "Invalid duration value: " + normalized);
            }
            if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
                throw ParseError(ParseError::InvalidParameter, "Extra characters after duration value: " + normalized);
            }
            return Duration::FromUnits(units);
        }
    }
    else {
        auto [den, res] = String::FromCLocaleTo<int>(normalized);
        if(res == String::FromCLocaleToState::Failed) {
            throw ParseError(ParseError::InvalidParameter, "Invalid duration value: " + normalized);
        }
        if(res == String::FromCLocaleToState::ScrapAtTheEnd) {
            throw ParseError(ParseError::InvalidParameter, "Extra characters after duration value: " + normalized);
        }
        return Duration::FromFraction(den);
    }

    throw ParseError(ParseError::InvalidToken, "Unrecognized duration token: " + std::string{token});
}

float Synth::Duration::ToSeconds(float tempo) const {
    switch(type) {
    case Fraction:
        return 240.0f / tempo * fraction.numerator / fraction.denominator;
    case Units:
        return 240.0f / tempo * units;
    case Seconds:
        return seconds;
    default:
        return 0.0f;
    }
}

size_t Synth::Duration::ToSamples(float tempo, float sample_rate) const {
    switch(type) {
    case Fraction:
        return static_cast<size_t>(240.0f / tempo * fraction.numerator / fraction.denominator * sample_rate);
    case Units:
        return static_cast<size_t>(240.0f / tempo * units * sample_rate);
    case Seconds:
        return static_cast<size_t>(seconds * sample_rate);
    default:
        return 0;
    }
}

} // namespace Gorgon::Audio