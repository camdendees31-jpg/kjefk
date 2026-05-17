// ─────────────────────────────────────────────────────────────────────────────
// Features.cpp  –  GTModMenu feature implementations
// ─────────────────────────────────────────────────────────────────────────────
#include "Features.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "GlobalNamespace/GorillaLocomotion/Player.hpp"
#include "GlobalNamespace/GorillaTagger.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Physics.hpp"
#include "UnityEngine/Rigidbody.hpp"
#include "UnityEngine/PrimitiveType.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Renderer.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/Mathf.hpp"

#include <vector>
#include <cmath>

using namespace UnityEngine;
using namespace GorillaLocomotion;

static ModLogger& logger() {
    static auto* l = new ModLogger();
    l->tag = "GTModMenu";
    return *l;
}

// ─── Spawned platform list ────────────────────────────────────────────────────
static std::vector<UnityEngine::GameObject*> s_platforms;

// ─── Rainbow hue state ────────────────────────────────────────────────────────
static float s_hue = 0.0f;

// ─── Utility: HSV → RGB ──────────────────────────────────────────────────────
static Color HsvToRgb(float h, float s, float v) {
    float r, g, b;
    int   i = (int)(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    switch (i % 6) {
        case 0: r=v; g=t; b=p; break;
        case 1: r=q; g=v; b=p; break;
        case 2: r=p; g=v; b=t; break;
        case 3: r=p; g=q; b=v; break;
        case 4: r=t; g=p; b=v; break;
        default:r=v; g=p; b=q; break;
    }
    return Color(r, g, b, 1.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick – called every frame
// ─────────────────────────────────────────────────────────────────────────────
void GTModMenu::Features::Tick(float deltaTime) {
    auto* player = Player::get_Instance();
    if (!player) return;

    auto* rb = player->get_GetComponent<Rigidbody*>();

    // ── Fly ──────────────────────────────────────────────────────────────────
    if (flyEnabled && rb) {
        // Dampen gravity, add vertical velocity when left trigger held
        auto vel = rb->get_velocity();
        vel.y = Mathf::Lerp(vel.y, flySpeed, deltaTime * 8.0f);
        rb->set_velocity(vel);
        rb->set_useGravity(false);
    } else if (rb) {
        rb->set_useGravity(true);
    }

    // ── Speed ────────────────────────────────────────────────────────────────
    if (speedEnabled) {
        player->maxJumpSpeed      = 12.0f * speedMultiplier;
        player->velocityLimit     = 12.0f * speedMultiplier;
    } else {
        player->maxJumpSpeed      = 12.0f;
        player->velocityLimit     = 12.0f;
    }

    // ── Bunny Hop ────────────────────────────────────────────────────────────
    if (bunnyhopEnabled && rb) {
        auto vel = rb->get_velocity();
        if (vel.y < -0.5f) vel.y = 4.5f;
        rb->set_velocity(vel);
    }

    // ── Noclip ───────────────────────────────────────────────────────────────
    // Disable player collision layer so geometry is phased through
    auto* go = player->get_gameObject();
    if (noclipEnabled) {
        go->get_layer(); // ensure loaded
        Physics::IgnoreLayerCollision(0, 0, true);
    } else {
        Physics::IgnoreLayerCollision(0, 0, false);
    }

    // ── Ghost (no player-to-player collision) ─────────────────────────────────
    if (ghostEnabled) {
        Physics::IgnoreLayerCollision(0, 8, true);
    } else {
        Physics::IgnoreLayerCollision(0, 8, false);
    }

    // ── Infinite Jump ────────────────────────────────────────────────────────
    if (infiniteJump) {
        player->jumpMultiplier = 2.0f;
    } else {
        player->jumpMultiplier = 1.0f;
    }

    // ── Rainbow colour ───────────────────────────────────────────────────────
    if (rainbowColor) {
        s_hue = fmodf(s_hue + deltaTime * 0.25f, 1.0f);
        Color c = HsvToRgb(s_hue, 1.0f, 1.0f);
        SetPlayerColor(c.r, c.g, c.b);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Platform helpers
// ─────────────────────────────────────────────────────────────────────────────
void GTModMenu::Features::SpawnPlatform() {
    auto* player = Player::get_Instance();
    if (!player) return;

    auto pos = player->get_transform()->get_position();
    pos.y -= 0.5f; // slightly below feet

    auto* plat = GameObject::CreatePrimitive(PrimitiveType::Cube);
    plat->get_transform()->set_position(pos);
    plat->get_transform()->set_localScale(Vector3(platformSize, 0.1f, platformSize));

    // Bright green material
    auto* rend = plat->get_GetComponent<Renderer*>();
    if (rend) {
        auto* mat = rend->get_material();
        mat->set_color(Color(0.0f, 1.0f, 0.3f, 1.0f));
    }

    s_platforms.push_back(plat);
}

void GTModMenu::Features::RemoveAllPlatforms() {
    for (auto* p : s_platforms) {
        if (p) GameObject::Destroy(p);
    }
    s_platforms.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// Colour helpers
// ─────────────────────────────────────────────────────────────────────────────
void GTModMenu::Features::SetPlayerColor(float r, float g, float b) {
    auto* tagger = GorillaTagger::get_Instance();
    if (!tagger) return;
    // GorillaTagger exposes the networked colour; set it and mark dirty
    tagger->offlineVRRig->mainSkin->set_color(Color(r, g, b, 1.0f));
}

void GTModMenu::Features::ResetColor() {
    SetPlayerColor(1.0f, 1.0f, 1.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Long arms
// ─────────────────────────────────────────────────────────────────────────────
void GTModMenu::Features::ApplyLongArms(bool enable) {
    auto* player = Player::get_Instance();
    if (!player) return;
    float scale = enable ? armScale : 1.0f;

    // Scale the hand transforms that GorillaLocomotion uses for input
    auto* lHand = player->leftHandTransform;
    auto* rHand = player->rightHandTransform;
    if (lHand) lHand->set_localScale(Vector3(scale, scale, scale));
    if (rHand) rHand->set_localScale(Vector3(scale, scale, scale));
}
