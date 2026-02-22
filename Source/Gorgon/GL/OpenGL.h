#pragma once

// Prevents GLEW from including windows.h itself on Windows
#define GLEW_NO_GLU

#include <GL/glew.h>

namespace Gorgon :: GL {
    void Initialize();
}
