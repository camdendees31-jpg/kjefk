#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Features.hpp  –  GTModMenu feature declarations
// ─────────────────────────────────────────────────────────────────────────────

namespace GTModMenu::Features {

// ── State toggles ─────────────────────────────────────────────────────────────
inline bool flyEnabled        = false;
inline bool noclipEnabled     = false;
inline bool speedEnabled      = false;
inline bool longArmsEnabled   = false;
inline bool ghostEnabled      = false;
inline bool infiniteJump      = false;
inline bool platformsEnabled  = false;
inline bool bunnyhopEnabled   = false;
inline bool rainbowColor      = false;

// ── Tunable values ────────────────────────────────────────────────────────────
inline float speedMultiplier  = 2.5f;   // applied on top of base speed
inline float flySpeed         = 4.0f;   // units/s vertical
inline float armScale         = 2.5f;   // multiplier on arm length
inline float platformSize     = 1.5f;   // metres

// ── Called every frame from the menu MonoBehaviour ────────────────────────────
void Tick(float deltaTime);

// ── One-shot helpers ──────────────────────────────────────────────────────────
void SpawnPlatform();
void RemoveAllPlatforms();
void SetPlayerColor(float r, float g, float b);
void ResetColor();
void ApplyLongArms(bool enable);

} // namespace GTModMenu::Features
