#pragma once

#include "Inputbox.h"

namespace Gorgon :: Widgets {
    
    /// This inputbox variant is designed to contain text. Unlike regular Textbox, this
    /// variant allows overriding the validator
    template<class T_ = float, class V_ = UI::ConversionValidator<float>>
    using NumberInputbox = Inputbox<T_, V_, NumericProperty>;
    
    /// An inputbox variant designed to real numbers.
    using Numberbox = Inputbox<float, UI::ConversionValidator<float>, NumericProperty>;
    
    /// An inputbox variant designed to integer numbers.
    using Integerbox = Inputbox<int, UI::ConversionValidator<int>, NumericProperty>;
    
}
