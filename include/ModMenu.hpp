#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// ModMenu.hpp  –  GTModMenu holdable in-world menu
// ─────────────────────────────────────────────────────────────────────────────
// The menu is a small physical slab that spawns in front of the player.
// Grab it with either controller grip, hold it wherever you like, and
// touch the coloured buttons to toggle features.
// Hold BOTH grips with NO headset movement for 2 s to summon / dismiss.
// ─────────────────────────────────────────────────────────────────────────────

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"

namespace GTModMenu {

// ─── MenuItem ─────────────────────────────────────────────────────────────────
struct MenuItem {
    const char*  label;
    bool*        toggle;          // nullptr → action button
    void       (*action)();       // called on press (can be nullptr for toggles)
    UnityEngine::GameObject* buttonObj = nullptr;
    UnityEngine::GameObject* labelObj  = nullptr;
};

// ─── ModMenu MonoBehaviour ────────────────────────────────────────────────────
class ModMenu {
public:
    static void Install();           // call once from main hook
    static void ShowMenu();
    static void HideMenu();
    static bool IsVisible();

private:
    static UnityEngine::GameObject* s_menuRoot;
    static bool                     s_visible;
    static float                    s_holdTimer;

    static void BuildMenuGeometry();
    static void RefreshButtonColors();
    static void HandleInput(float dt);
};

} // namespace GTModMenu
