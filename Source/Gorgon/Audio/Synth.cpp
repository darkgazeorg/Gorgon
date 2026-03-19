#include "Synth.h"
#include "Gorgon/String.h"

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

Synth::Node Synth::Node::MakeVolume(float volume) {
    Node n;

    n.type = Type::Volume;
    n.volume = volume;

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

Synth::Node Synth::ParseNode(const std::string_view& token) {
    if(token.empty()) return {};

    std::string normalized = String::ToLower(String::Trim(std::string{token}));
        
    switch(normalized[0]) {
    case 't': 
        return Node::MakeTempo(std::stof(normalized.substr(1)));
    case 'o': {
        auto [oct, res] = String::FromCLocaleTo<int>(normalized.substr(1));
        if(res == String::CLocateToResultStates::Failed) {
            throw ParseError(ParseError::InvalidParameter, "Invalid octave value: " + normalized.substr(1));
        }
        if(res == String::CLocateToResultStates::ScrapAtTheEnd) {
            throw ParseError(ParseError::InvalidParameter, "Extra characters after octave value: " + normalized.substr(1));
        }
        return Node::MakeOctaveAbsolute(oct);
    }
    case '>':
        return Node::MakeOctaveRelative(1);
    case '<':
        return Node::MakeOctaveRelative(-1);
    case 'v': {
        auto [vol, res] = String::FromCLocaleTo<double>(normalized.substr(1));

        if(res == String::CLocateToResultStates::Failed) {
            throw ParseError(ParseError::InvalidParameter, "Invalid volume value: " + normalized.substr(1));
        }
        if(res == String::CLocateToResultStates::ScrapAtTheEnd) {
            throw ParseError(ParseError::InvalidParameter, "Extra characters after volume value: " + normalized.substr(1));
        }
        if(vol < 0 || vol > 100) {
            throw ParseError(ParseError::InvalidParameter, "Volume must be between 0 and 100: " + std::to_string(vol));
        }

        return Node::MakeVolume(vol / 100.0f);
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
    std::string line;

    while(std::getline(stream, line)) {
        // remove comments
        auto commentPos = line.find('#');
        if(commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        if(String::Trim(line).empty()) continue;

        auto tokens = String::Split(line, ' ');

        for(const auto& token : tokens) {
            if(String::Trim(token).empty()) continue;
            Nodes.push_back(ParseNode(token));
        }
    }
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
        std::tie(den, res) = String::FromCLocaleTo<int>(normalized);
        return Duration::FromFraction(nom, den);
    }
    
    if(normalized[0] == '(') {
        if(normalized.back() != ')') {
            throw ParseError(ParseError::InvalidToken, "Mismatched parentheses in duration token: " + std::string{token});
        }

        return Duration::FromSeconds(String::To<float>(normalized.substr(1, normalized.size() - 2)));
    }

    // this last state is 3 in 1. If ends with dot, it's a fraction with 1.5 multiplier.
    // if it has a dot in the middle, it's unit based duration. Otherwise, it's a simple fraction.
    if(auto pos = normalized.find('.'); pos != std::string::npos) {
        if(pos == normalized.size() - 1) {
            auto [base, res] = String::FromCLocaleTo<int>(normalized.substr(0, pos));
            if(res == String::CLocateToResultStates::Failed) {
                throw ParseError(ParseError::InvalidParameter, "Invalid duration value: " + normalized.substr(0, pos));
            }
            if(res == String::CLocateToResultStates::ScrapAtTheEnd) {
                throw ParseError(ParseError::InvalidParameter, "Extra characters after duration value: " + normalized.substr(0, pos));
            }
            return Duration::FromFraction(3, base * 2);
        }
        else {
            auto [units, res] = String::FromCLocaleTo<float>(normalized);
            if(res == String::CLocateToResultStates::Failed) {
                throw ParseError(ParseError::InvalidParameter, "Invalid duration value: " + normalized);
            }
            if(res == String::CLocateToResultStates::ScrapAtTheEnd) {
                throw ParseError(ParseError::InvalidParameter, "Extra characters after duration value: " + normalized);
            }
            return Duration::FromUnits(units);
        }
    }
    else {
        auto [den, res] = String::FromCLocaleTo<int>(normalized);
        if(res == String::CLocateToResultStates::Failed) {
            throw ParseError(ParseError::InvalidParameter, "Invalid duration value: " + normalized);
        }
        if(res == String::CLocateToResultStates::ScrapAtTheEnd) {
            throw ParseError(ParseError::InvalidParameter, "Extra characters after duration value: " + normalized);
        }
        return Duration::FromFraction(den);
    }

    throw ParseError(ParseError::InvalidToken, "Unrecognized duration token: " + std::string{token});
}

} // namespace Gorgon::Audio