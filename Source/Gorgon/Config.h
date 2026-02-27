#pragma once

/** @defgroup config "Gorgon Configuration" 
 * These variables control the behavior of Gorgon Library. Unless defined as constant they 
 * can be changed. However, changing configuration on the fly might have unexpected 
 * consequences. Therefore, it is advisable to adjust settings before initializing the
 * library. When compiling Gorgon, the default values can be set by compiler definitions
 * which also contains namespaces separated by underscore. Namespace and variable names
 * should all be capitalized. For instance, if you want to modify ClickThreshold in
 * WindowManager namespace, you should define GORGON_WINDOWMANAGER_CLICKTHRESHOLD.
 */

namespace Gorgon :: WindowManager {
    
    /// The maximum distance allowed for mouse to move between the press of the button and
    /// the release for click event to register. Default value is 5.
    extern int ClickThreshold;
    
}
    
    
