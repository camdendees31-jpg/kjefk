// ─────────────────────────────────────────────────────────────────────────────
// main.cpp  –  Scotland2 mod entry point for GTModMenu
// ─────────────────────────────────────────────────────────────────────────────
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "ModMenu.hpp"

// Scotland2 uses __attribute__((constructor)) for mod load
__attribute__((visibility("default")))
__attribute__((used))
extern "C" void setup(CModInfo* info) {
    // Fill in mod metadata (matches mod.json)
    info->id      = "gtmodmenu";
    info->version = "1.0.0";

    Paper::Logger::fmtLog<Paper::LogLevel::INF, "GTModMenu">(
        "GTModMenu v1.0.0 – loading hooks …"
    );
}

// Called after il2cpp is fully initialised
__attribute__((visibility("default")))
__attribute__((used))
extern "C" void load() {
    Paper::Logger::fmtLog<Paper::LogLevel::INF, "GTModMenu">(
        "Installing hooks …"
    );

    // Install the Player::Update hook (defined in ModMenu.cpp)
    INSTALL_HOOK(Paper::Logger::get(), Player_Update);

    // Build the physical menu geometry (deferred until first frame hits)
    // We schedule it via a coroutine started inside the hook instead of here,
    // because il2cpp types aren't ready until the first scene loads.
    // ModMenu::Install() is called from inside Player_Update on first call.
    static bool firstCall = true;
    (void)firstCall; // handled inside hook

    Paper::Logger::fmtLog<Paper::LogLevel::INF, "GTModMenu">(
        "All hooks installed – hold BOTH grips for 2 s in-game to open menu!"
    );
}
