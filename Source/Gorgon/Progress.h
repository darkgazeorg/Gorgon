#pragma once

#include "Gorgon/Containers/Collection.h"
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

    class MultiTargetProgressDisplay : public ProgressDisplay {
    public:
        void SetProgressRatio(Float value) override {
            for(auto &target : targets) {
                target.SetProgressRatio(value);
            }
        }

        Float GetProgressRatio() const override {
            if(targets.GetSize() == 0) return -1.f;

            Float sum = 0.f;
            for(auto &target : targets) {
                sum += target.GetProgressRatio();
            }

            return sum / targets.GetSize();
        }
        
        /// Adds a new target to the progress display. The progress
        /// sent to this display will be forwarded to all targets.
        virtual void AddTarget(ProgressDisplay &target) {
            targets.Add(target);
        }

        /// Removes a target from the progress display.
        virtual void RemoveTarget(ProgressDisplay &target) {
            targets.Remove(target);
        }

    private:
        Containers::Collection<ProgressDisplay> targets;
    };

}
