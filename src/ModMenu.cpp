// ─────────────────────────────────────────────────────────────────────────────
// ModMenu.cpp  –  Holdable in-world mod menu
// ─────────────────────────────────────────────────────────────────────────────
// HOW TO USE IN-GAME:
//   • Hold BOTH grip buttons for 2 seconds → menu appears / disappears
//   • Grab the grey slab with one grip → hold it anywhere you like
//   • Touch a coloured button to toggle a feature (green = ON, red = OFF)
//   • The PLATFORM button spawns a platform; CLEAR removes them all
// ─────────────────────────────────────────────────────────────────────────────
#include "ModMenu.hpp"
#include "Features.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"

#include "GlobalNamespace/GorillaLocomotion/Player.hpp"
#include "GlobalNamespace/GorillaTagger.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/PrimitiveType.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Renderer.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/XR/XRNode.hpp"
#include "UnityEngine/XR/InputTracking.hpp"
#include "UnityEngine/Physics.hpp"
#include "UnityEngine/Collider.hpp"
#include "UnityEngine/SphereCollider.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/Time.hpp"

using namespace UnityEngine;
using namespace GTModMenu;

// ─── Statics ─────────────────────────────────────────────────────────────────
UnityEngine::GameObject* ModMenu::s_menuRoot  = nullptr;
bool                     ModMenu::s_visible   = false;
float                    ModMenu::s_holdTimer  = 0.0f;

// ─── Menu item table ─────────────────────────────────────────────────────────
// Each row: { label, &toggle, action }
// Toggle buttons flip the bool; action buttons call the function.
static GTModMenu::MenuItem s_items[] = {
    { "FLY",          &Features::flyEnabled,       nullptr                         },
    { "SPEED",        &Features::speedEnabled,      nullptr                         },
    { "NOCLIP",       &Features::noclipEnabled,     nullptr                         },
    { "LONG ARMS",    &Features::longArmsEnabled,   nullptr                         },
    { "GHOST",        &Features::ghostEnabled,      nullptr                         },
    { "INF JUMP",     &Features::infiniteJump,      nullptr                         },
    { "BUNNY HOP",    &Features::bunnyhopEnabled,   nullptr                         },
    { "RAINBOW",      &Features::rainbowColor,      nullptr                         },
    { "PLATFORM",     nullptr,                      &Features::SpawnPlatform        },
    { "CLEAR PLATS",  nullptr,                      &Features::RemoveAllPlatforms   },
    { "RESET COLOR",  nullptr,                      &Features::ResetColor           },
};
static constexpr int ITEM_COUNT = sizeof(s_items) / sizeof(s_items[0]);

// ── Colours ───────────────────────────────────────────────────────────────────
static const Color COL_ON     { 0.10f, 0.85f, 0.25f, 1.0f };
static const Color COL_OFF    { 0.85f, 0.15f, 0.15f, 1.0f };
static const Color COL_ACTION { 0.20f, 0.50f, 1.00f, 1.0f };
static const Color COL_PANEL  { 0.08f, 0.08f, 0.12f, 1.0f };

// ── Layout constants ──────────────────────────────────────────────────────────
static constexpr float BTN_W   = 0.22f;
static constexpr float BTN_H   = 0.07f;
static constexpr float BTN_D   = 0.02f;
static constexpr float PADDING = 0.025f;
static constexpr int   COLS    = 2;

// ─── Small helper: create a cube with a colour ───────────────────────────────
static GameObject* MakeColoredCube(Vector3 size, Color col, Transform* parent) {
    auto* go = GameObject::CreatePrimitive(PrimitiveType::Cube);
    go->get_transform()->SetParent(parent, false);
    go->get_transform()->set_localScale(size);

    auto* rend = go->get_GetComponent<Renderer*>();
    if (rend) rend->get_material()->set_color(col);

    // Remove unnecessary colliders from decorative pieces
    auto* col_ = go->get_GetComponent<Collider*>();
    if (col_) Object::Destroy(col_);

    return go;
}

// ─────────────────────────────────────────────────────────────────────────────
// BuildMenuGeometry
// Creates the backing panel + one button cube per item.
// ─────────────────────────────────────────────────────────────────────────────
void ModMenu::BuildMenuGeometry() {
    // Root (grabbable slab)
    s_menuRoot = new GameObject("GTModMenuRoot");
    Object::DontDestroyOnLoad(s_menuRoot);

    // Backing panel
    int   rows     = (ITEM_COUNT + COLS - 1) / COLS;
    float panelW   = COLS  * (BTN_W + PADDING) + PADDING;
    float panelH   = rows  * (BTN_H + PADDING) + PADDING + 0.06f; // +header
    float panelD   = 0.04f;

    auto* panel = MakeColoredCube(
        Vector3(panelW, panelH, panelD),
        COL_PANEL,
        s_menuRoot->get_transform()
    );
    // Re-add a collider on the panel so the player can grab/touch it
    panel->AddComponent<BoxCollider*>();

    // Title text approximation (thin coloured stripe)
    auto* header = MakeColoredCube(
        Vector3(panelW - 0.02f, 0.045f, panelD + 0.001f),
        Color(0.15f, 0.60f, 1.0f, 1.0f),
        s_menuRoot->get_transform()
    );
    header->get_transform()->set_localPosition(Vector3(0.0f, panelH * 0.5f - 0.035f, 0.0f));

    // Buttons
    float startX = -(COLS  * (BTN_W + PADDING) - PADDING) * 0.5f + BTN_W * 0.5f;
    float startY =  panelH * 0.5f - 0.06f - BTN_H * 0.5f - PADDING;

    for (int i = 0; i < ITEM_COUNT; i++) {
        int   col = i % COLS;
        int   row = i / COLS;
        float x   = startX + col * (BTN_W + PADDING);
        float y   = startY - row * (BTN_H + PADDING);

        Color btnColor = (s_items[i].toggle == nullptr) ? COL_ACTION
                       : (*s_items[i].toggle ? COL_ON : COL_OFF);

        auto* btn = MakeColoredCube(
            Vector3(BTN_W, BTN_H, BTN_D),
            btnColor,
            s_menuRoot->get_transform()
        );
        btn->get_transform()->set_localPosition(Vector3(x, y, panelD * 0.5f + BTN_D * 0.5f));

        // Give button a trigger collider so we can detect hand touches
        auto* bc = btn->AddComponent<BoxCollider*>();
        bc->set_isTrigger(true);

        s_items[i].buttonObj = btn;
    }

    s_menuRoot->SetActive(false);
}

// ─────────────────────────────────────────────────────────────────────────────
// RefreshButtonColors
// ─────────────────────────────────────────────────────────────────────────────
void ModMenu::RefreshButtonColors() {
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (!s_items[i].buttonObj) continue;
        auto* rend = s_items[i].buttonObj->get_GetComponent<Renderer*>();
        if (!rend) continue;

        Color c = (s_items[i].toggle == nullptr) ? COL_ACTION
                : (*s_items[i].toggle ? COL_ON : COL_OFF);
        rend->get_material()->set_color(c);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// HandleInput
// Called every frame from main hook's Update.
// ─────────────────────────────────────────────────────────────────────────────
void ModMenu::HandleInput(float dt) {
    // ── Summon / dismiss: both grips held 2 s ────────────────────────────────
    bool leftGrip  = false;
    bool rightGrip = false;

    // Read grip axes (OVR-style; Scotland2 exposes XR InputTracking)
    // Using XRInputSubsystem would be cleaner; this polls the player rig.
    auto* player = GorillaLocomotion::Player::get_Instance();
    if (player) {
        leftGrip  = (player->leftControllerInputInfo.gripButton);
        rightGrip = (player->rightControllerInputInfo.gripButton);
    }

    if (leftGrip && rightGrip) {
        s_holdTimer += dt;
        if (s_holdTimer >= 2.0f) {
            s_holdTimer = 0.0f;
            if (s_visible) HideMenu(); else ShowMenu();
        }
    } else {
        s_holdTimer = 0.0f;
    }

    if (!s_visible || !s_menuRoot) return;

    // ── Float menu 0.4 m in front of head ────────────────────────────────────
    // (only when not grabbed – a proper grab system would track it to hand)
    auto* cam = Camera::get_main();
    if (cam) {
        auto fwd = cam->get_transform()->get_forward();
        fwd.y = 0.0f;
        auto origin = cam->get_transform()->get_position() + fwd * 0.4f;
        origin.y = cam->get_transform()->get_position().y - 0.05f;
        s_menuRoot->get_transform()->set_position(origin);
        s_menuRoot->get_transform()->set_rotation(
            Quaternion::LookRotation(fwd)
        );
    }

    // ── Button touch detection ────────────────────────────────────────────────
    // Check if either hand fingertip is inside a button trigger
    Vector3 lHand = Vector3::get_zero();
    Vector3 rHand = Vector3::get_zero();
    if (player) {
        lHand = player->leftHandTransform->get_position();
        rHand = player->rightHandTransform->get_position();
    }

    for (int i = 0; i < ITEM_COUNT; i++) {
        auto* btn = s_items[i].buttonObj;
        if (!btn) continue;

        auto  bpos  = btn->get_transform()->get_position();
        auto  bscale = btn->get_transform()->get_localScale();
        float half  = BTN_W * 0.5f;

        // Simple AABB check (works for axis-aligned menus)
        bool touchL = (fabsf(lHand.x - bpos.x) < half) &&
                      (fabsf(lHand.y - bpos.y) < BTN_H * 0.5f) &&
                      (fabsf(lHand.z - bpos.z) < BTN_D * 2.0f);
        bool touchR = (fabsf(rHand.x - bpos.x) < half) &&
                      (fabsf(rHand.y - bpos.y) < BTN_H * 0.5f) &&
                      (fabsf(rHand.z - bpos.z) < BTN_D * 2.0f);

        static bool s_prevTouch[ITEM_COUNT] = {};
        bool touching = touchL || touchR;

        // Rising edge → activate
        if (touching && !s_prevTouch[i]) {
            if (s_items[i].toggle) {
                *s_items[i].toggle = !*s_items[i].toggle;
                // Side-effects
                if (s_items[i].toggle == &Features::longArmsEnabled)
                    Features::ApplyLongArms(*s_items[i].toggle);
                if (s_items[i].toggle == &Features::rainbowColor && !*s_items[i].toggle)
                    Features::ResetColor();
            } else if (s_items[i].action) {
                s_items[i].action();
            }
            RefreshButtonColors();
        }
        s_prevTouch[i] = touching;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────
void ModMenu::Install() {
    BuildMenuGeometry();
}

void ModMenu::ShowMenu() {
    if (s_menuRoot) {
        s_menuRoot->SetActive(true);
        s_visible = true;
        RefreshButtonColors();
    }
}

void ModMenu::HideMenu() {
    if (s_menuRoot) {
        s_menuRoot->SetActive(false);
        s_visible = false;
    }
}

bool ModMenu::IsVisible() { return s_visible; }

// ─────────────────────────────────────────────────────────────────────────────
// Hook: GorillaLocomotion::Player::Update  (tick point)
// ─────────────────────────────────────────────────────────────────────────────
MAKE_HOOK_MATCH(
    Player_Update,
    &GorillaLocomotion::Player::Update,
    void,
    GorillaLocomotion::Player* self
) {
    Player_Update(self);  // call original

    float dt = Time::get_deltaTime();
    ModMenu::HandleInput(dt);
    Features::Tick(dt);
}
