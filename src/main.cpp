#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "A90State.hpp"

#include <algorithm>
#include <random>

using namespace geode::prelude;

namespace {
float randomFloat(float a, float b) {
    static std::mt19937 rng{std::random_device{}()};
    return std::uniform_real_distribution<float>(a, b)(rng);
}

void removeUI(a90::State* s) {
    if (s && s->ui) {
        s->ui->removeFromParentAndCleanup(true);
        s->ui = nullptr;
    }
}

cocos2d::CCSprite* loadSprite(char const* file) {
    auto path = Mod::get()->getResourcesDir() / file;
    return cocos2d::CCSprite::create(path.string().c_str());
}

void playSound(char const* file) {
    auto path = Mod::get()->getResourcesDir() / file;
    FMODAudioEngine::sharedEngine()->playEffect(path.string());
}

void addSpriteCentered(cocos2d::CCNode* root, cocos2d::CCSprite* sprite, float fraction) {
    if (!sprite) return;
    auto size = cocos2d::CCDirector::sharedDirector()->getVisibleSize();
    sprite->setPosition({size.width / 2.f, size.height / 2.f});
    float scale = std::min(
        size.width / sprite->getContentSize().width,
        size.height / sprite->getContentSize().height
    ) * fraction;
    sprite->setScale(scale);
    root->addChild(sprite);
}

void showWarning(a90::State* s) {
    removeUI(s);

    auto scene = cocos2d::CCDirector::sharedDirector()->getRunningScene();
    if (!scene) return;

    auto size = cocos2d::CCDirector::sharedDirector()->getVisibleSize();
    auto origin = cocos2d::CCDirector::sharedDirector()->getVisibleOrigin();

    auto root = cocos2d::CCLayerColor::create({0,0,0,0}, size.width, size.height);
    if (!root) return;

    root->setPosition(origin);
    root->setZOrder(1000000);

    addSpriteCentered(root, loadSprite("A-90.webp"), 0.72f);
    addSpriteCentered(root, loadSprite("A-90_Stop_Sign.webp"), 0.48f);

    scene->addChild(root);
    s->ui = root;

    playSound("A-90_Warning.mp3");
}

void showJumpscare(a90::State* s) {
    removeUI(s);

    auto scene = cocos2d::CCDirector::sharedDirector()->getRunningScene();
    if (!scene) return;

    auto size = cocos2d::CCDirector::sharedDirector()->getVisibleSize();
    auto origin = cocos2d::CCDirector::sharedDirector()->getVisibleOrigin();

    auto root = cocos2d::CCLayerColor::create({0,0,0,0}, size.width, size.height);
    if (!root) return;

    root->setPosition(origin);
    root->setZOrder(1000001);

    auto image = loadSprite("A-90_Jumpscare.webp");
    if (image) {
        image->setPosition({size.width / 2.f, size.height / 2.f});
        float scale = std::max(
            size.width / image->getContentSize().width,
            size.height / image->getContentSize().height
        );
        image->setScale(scale);
        root->addChild(image);
    }

    scene->addChild(root);
    s->ui = root;
    playSound("A-90_Jumpscare.mp3");
}

bool gameplayInput(int button) {
    switch (static_cast<PlayerButton>(button)) {
        case PlayerButton::Jump:
        case PlayerButton::Left:
        case PlayerButton::Right:
            return true;
        default:
            return false;
    }
}
}

class $modify(A90PlayLayer, PlayLayer) {
public:
    struct Fields { a90::State state; };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;
        m_fields->state = {};
        return true;
    }

    void startGame() {
        PlayLayer::startGame();
        auto& s = m_fields->state;
        removeUI(&s);
        s = {};
        s.levelActive = true;
        s.triggerTimer = randomFloat(a90::TRIGGER_DELAY_MIN, a90::TRIGGER_DELAY_MAX);
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto& s = m_fields->state;

        if (!s.levelActive || s.dead || m_isPaused)
            return;

        if (s.phase == a90::Phase::Inactive) {
            s.triggerTimer -= dt;
            if (s.triggerTimer <= 0.f) {
                s.phase = a90::Phase::Stop;
                s.stopTimer = 0.f;
                showWarning(&s);
            }
        } else {
            s.stopTimer += dt;
            if (s.stopTimer >= a90::STOP_DURATION) {
                removeUI(&s);
                s.phase = a90::Phase::Inactive;
                s.stopTimer = 0.f;
                s.triggerTimer = randomFloat(
                    a90::TRIGGER_DELAY_MIN,
                    a90::TRIGGER_DELAY_MAX
                );
            }
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        auto& s = m_fields->state;
        s.dead = true;
        s.levelActive = false;
        removeUI(&s);
        s.phase = a90::Phase::Inactive;
        s.stopTimer = 0.f;
        PlayLayer::destroyPlayer(player, object);
    }

    void resetLevel() {
        auto& s = m_fields->state;
        removeUI(&s);
        s = {};
        s.levelActive = true;
        s.triggerTimer = randomFloat(
            a90::TRIGGER_DELAY_MIN,
            a90::TRIGGER_DELAY_MAX
        );
        PlayLayer::resetLevel();
    }

    void levelComplete() {
        auto& s = m_fields->state;
        s.levelActive = false;
        removeUI(&s);
        PlayLayer::levelComplete();
    }

    void onQuit() {
        auto& s = m_fields->state;
        s.levelActive = false;
        s.dead = true;
        removeUI(&s);
        PlayLayer::onQuit();
    }

    ~A90PlayLayer() {
        removeUI(&m_fields->state);
    }
};

class $modify(A90InputLayer, GJBaseGameLayer) {
public:
    void handleButton(bool down, int button, bool player1) {
        auto* playLayer = typeinfo_cast<A90PlayLayer*>(this);

        if (playLayer) {
            auto& s = playLayer->m_fields->state;

            if (down && s.levelActive && !s.dead &&
                s.phase == a90::Phase::Stop && gameplayInput(button)) {

                auto player = player1 ? playLayer->m_player1 : playLayer->m_player2;

                s.dead = true;
                s.levelActive = false;
                s.phase = a90::Phase::Inactive;
                s.stopTimer = 0.f;

                showJumpscare(&s);

                if (player) {
                    playLayer->destroyPlayer(player, nullptr);
                }

                // Don't forward the input that failed A-90.
                return;
            }
        }

        GJBaseGameLayer::handleButton(down, button, player1);
    }
};
