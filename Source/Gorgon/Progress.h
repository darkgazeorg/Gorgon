#pragma once

#include "Types.h"

namespace Gorgon {

    /**
     * Interface for any visual or hardware element that can display progress
     * as a normalized ratio between 0.0 (empty/start) and 1.0 (full/complete).
     */
    class ProgressDisplay {
    public:
        virtual ~ProgressDisplay() = default;

        /// Sets the current progress ratio in the range [0.0, 1.0].
        virtual void SetProgressRatio(Float value) = 0;

        /// Returns the current progress ratio in [0.0, 1.0], or -1.0 if unavailable.
        virtual Float GetProgressRatio() const {
            return -1.f;
        }
    };

}
