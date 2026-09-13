#pragma once
#include <Geode/Geode.hpp>

namespace a90 {
enum class Phase { Inactive, Stop };

struct State {
    Phase phase = Phase::Inactive;
    float triggerTimer = 0.f;
    float stopTimer = 0.f;
    bool levelActive = false;
    bool dead = false;
    cocos2d::CCNode* ui = nullptr;
};

constexpr float TRIGGER_DELAY_MIN = 8.0f;
constexpr float TRIGGER_DELAY_MAX = 25.0f;
constexpr float STOP_DURATION = 0.65f;
constexpr float JUMPSCARE_TIME = 0.22f;
}
