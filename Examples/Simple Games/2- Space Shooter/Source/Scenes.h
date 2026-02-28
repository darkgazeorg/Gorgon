#pragma once

// =============================================================================
// Scenes.h
// -----------------------------------------------------------------------------
// Games typically have multiple distinct "screens": a main menu, the actual
// gameplay, a game-over screen, pause screen, etc. The engine calls each of
// these a Scene.
//
// Every scene needs a unique ID so the scene manager knows which one to show.
// We define those IDs here as constants so every file can refer to them by
// name instead of a raw number like 0 or 1. Using named constants avoids
// confusion and makes the code much easier to read.
// =============================================================================

#include <Gorgon/Scene.h>

// "constexpr" means the value is fixed at compile time - it will never change
// while the game is running. This is preferred over a plain "#define" because
// it has a proper type and plays nicely with the debugger.
constexpr Gorgon::SceneID MAIN_MENU  = 0;  // The first screen the player sees
constexpr Gorgon::SceneID GAME_SCENE = 1;  // The actual gameplay screen
