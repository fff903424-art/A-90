#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "A90State.hpp"

#include <algorithm>
#include <random>
#include <vector>

using namespace geode::prelude;

namespace {

constexpr float A90_MOVE_TIME = 0.32f;
constexpr int STATIC_LINES = 70;
constexpr float STATIC_REFRESH_TIME = 0.025f;
constexpr float SPAWN_MARGIN = 190.0f;

std::mt19937& rng() {
    static std::mt19937 generator{std::random_device{}()};
    return generator;
}

float randomFloat(float min, float max) {
    return std::uniform_real_distribution<float>(min, max)(rng());
}

int randomInt(int min, int max) {
    return std::uniform_int_distribution<int>(min, max)(rng());
}

void removeA90UI(a90::State* state) {
    if (!state || !state->ui)
        return;

    state->ui->removeFromParentAndCleanup(true);
    state->ui = nullptr;
}

cocos2d::CCSprite* loadSprite(const char* filename) {
    auto path = Mod::get()->getResourcesDir() / filename;
    return cocos2d::CCSprite::create(path.string().c_str());
}

void playSound(const char* filename) {
    auto path = Mod::get()->getResourcesDir() / filename;
    FMODAudioEngine::sharedEngine()->playEffect(path.string());
}

class A90StaticLayer : public cocos2d::CCNode {
    struct StaticPiece {
        cocos2d::CCLayerColor* node;
        float width;
        float height;
    };

    std::vector<StaticPiece> m_pieces;
    float m_refreshTimer = 0.0f;

public:
    static A90StaticLayer* create(cocos2d::CCSize size) {
        auto layer = new A90StaticLayer();

        if (layer && layer->init(size)) {
            layer->autorelease();
            return layer;
        }

        CC_SAFE_DELETE(layer);
        return nullptr;
    }

    bool init(cocos2d::CCSize size) {
        if (!CCNode::init())
            return false;

        setContentSize(size);

        for (int i = 0; i < STATIC_LINES; ++i) {
            float width = randomFloat(
                size.width * 0.15f,
                size.width * 0.90f
            );

            float height = randomFloat(2.0f, 10.0f);

            auto piece = cocos2d::CCLayerColor::create(
                {255, 0, 0, 0},
                width,
                height
            );

            if (!piece)
                continue;

            piece->setAnchorPoint({0.0f, 0.0f});
            addChild(piece);

            m_pieces.push_back({
                piece,
                width,
                height
            });
        }

        refresh();
        scheduleUpdate();

        return true;
    }

    void refresh() {
        auto size = getContentSize();

        for (auto& piece : m_pieces) {
            if (!piece.node)
                continue;

            float x = randomFloat(
                -20.0f,
                std::max(0.0f, size.width - piece.width)
            );

            float y = randomFloat(
                0.0f,
                std::max(0.0f, size.height - piece.height)
            );

            piece.node->setPosition({x, y});

            int type = randomInt(0, 9);

            cocos2d::ccColor3B color;

            if (type <= 6) {
                color = {
                    static_cast<GLubyte>(randomInt(90, 255)),
                    static_cast<GLubyte>(randomInt(0, 30)),
                    static_cast<GLubyte>(randomInt(0, 30))
                };
            }
            else if (type <= 8) {
                color = {
                    255,
                    static_cast<GLubyte>(randomInt(15, 80)),
                    static_cast<GLubyte>(randomInt(15, 80))
                };
            }
            else {
                color = {255, 255, 255};
            }

            piece.node->setColor(color);
            piece.node->setOpacity(
                static_cast<GLubyte>(randomInt(35, 155))
            );
            piece.node->setScaleX(randomFloat(0.6f, 2.2f));
        }
    }

    void update(float dt) override {
        m_refreshTimer += dt;

        if (m_refreshTimer >= STATIC_REFRESH_TIME) {
            m_refreshTimer = 0.0f;
            refresh();
        }
    }
};

void showA90Warning(a90::State* state) {
    removeA90UI(state);

    auto director = cocos2d::CCDirector::sharedDirector();
    auto scene = director->getRunningScene();

    if (!scene)
        return;

    auto visibleSize = director->getVisibleSize();
    auto origin = director->getVisibleOrigin();

    auto root = cocos2d::CCLayerColor::create(
        {0, 0, 0, 0},
        visibleSize.width,
        visibleSize.height
    );

    if (!root)
        return;

    root->setPosition(origin);
    root->setZOrder(1000000);

    auto staticLayer = A90StaticLayer::create(visibleSize);

    if (staticLayer)
        root->addChild(staticLayer, 0);

    auto face = loadSprite("A-90.webp");

    if (face) {
        auto faceSize = face->getContentSize();

        float scale = std::min(
            visibleSize.width / faceSize.width,
            visibleSize.height / faceSize.height
        ) * 0.55f;

        face->setScale(scale);

        float margin = std::min(
            SPAWN_MARGIN,
            std::min(
                visibleSize.width * 0.25f,
                visibleSize.height * 0.25f
            )
        );

        float minX = margin;
        float maxX = visibleSize.width - margin;
        float minY = margin;
        float maxY = visibleSize.height - margin;

        if (maxX < minX) {
            minX = visibleSize.width * 0.5f;
            maxX = minX;
        }

        if (maxY < minY) {
            minY = visibleSize.height * 0.5f;
            maxY = minY;
        }

        cocos2d::CCPoint startPosition = {
            randomFloat(minX, maxX),
            randomFloat(minY, maxY)
        };

        cocos2d::CCPoint centerPosition = {
            visibleSize.width * 0.5f,
            visibleSize.height * 0.5f
        };

        face->setPosition(startPosition);

        face->runAction(
            cocos2d::CCMoveTo::create(
                A90_MOVE_TIME,
                centerPosition
            )
        );

        root->addChild(face, 10);
    }

    auto stopSign = loadSprite("A-90_Stop_Sign.webp");

    if (stopSign) {
        auto signSize = stopSign->getContentSize();

        float scale = std::min(
            visibleSize.width / signSize.width,
            visibleSize.height / signSize.height
        ) * 0.42f;

        stopSign->setScale(scale);

        stopSign->setPosition({
            visibleSize.width * 0.5f,
            visibleSize.height * 0.5f
        });

        stopSign->setOpacity(0);

        auto delay = cocos2d::CCDelayTime::create(
            A90_MOVE_TIME * 0.65f
        );

        auto fadeIn = cocos2d::CCFadeIn::create(0.10f);

        stopSign->runAction(
            cocos2d::CCSequence::create(
                delay,
                fadeIn,
                nullptr
            )
        );

        root->addChild(stopSign, 20);
    }

    scene->addChild(root);
    state->ui = root;

    playSound("A-90_Warning.mp3");
}

void showA90Jumpscare(a90::State* state) {
    removeA90UI(state);

    auto director = cocos2d::CCDirector::sharedDirector();
    auto scene = director->getRunningScene();

    if (!scene)
        return;

    auto visibleSize = director->getVisibleSize();
    auto origin = director->getVisibleOrigin();

    auto root = cocos2d::CCLayerColor::create(
        {0, 0, 0, 0},
        visibleSize.width,
        visibleSize.height
    );

    if (!root)
        return;

    root->setPosition(origin);
    root->setZOrder(1000001);

    auto staticLayer = A90StaticLayer::create(visibleSize);

    if (staticLayer)
        root->addChild(staticLayer, 0);

    auto jumpscare = loadSprite("A-90_Jumpscare.webp");

    if (jumpscare) {
        auto imageSize = jumpscare->getContentSize();

        float scale = std::max(
            visibleSize.width / imageSize.width,
            visibleSize.height / imageSize.height
        );

        scale *= 1.08f;

        jumpscare->setScale(scale);

        jumpscare->setPosition({
            visibleSize.width * 0.5f,
            visibleSize.height * 0.5f
        });

        root->addChild(jumpscare, 10);
    }

    scene->addChild(root);
    state->ui = root;

    playSound("A-90_Jumpscare.mp3");
}

bool isGameplayInput(int button) {
    return button == 1 ||
           button == 2 ||
           button == 3;
}

}

class $modify(A90PlayLayer, PlayLayer) {
public:
    struct Fields {
        a90::State state;
    };

    bool init(
        GJGameLevel* level,
        bool useReplay,
        bool dontCreateObjects
    ) {
        if (!PlayLayer::init(
            level,
            useReplay,
            dontCreateObjects
        )) {
            return false;
        }

        m_fields->state = {};

        return true;
    }

    void startGame() {
        PlayLayer::startGame();

        auto& state = m_fields->state;

        removeA90UI(&state);

        state.phase = a90::Phase::Inactive;
        state.stopTimer = 0.0f;
        state.levelActive = true;
        state.dead = false;

        state.triggerTimer = randomFloat(
            a90::TRIGGER_DELAY_MIN,
            a90::TRIGGER_DELAY_MAX
        );
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        auto& state = m_fields->state;

        if (!state.levelActive ||
            state.dead ||
            m_isPaused) {
            return;
        }

        if (state.phase == a90::Phase::Inactive) {
            state.triggerTimer -= dt;

            if (state.triggerTimer <= 0.0f) {
                state.phase = a90::Phase::Stop;
                state.stopTimer = -A90_MOVE_TIME;

                showA90Warning(&state);
            }

            return;
        }

        if (state.phase == a90::Phase::Stop &&
            state.stopTimer < 0.0f) {

            state.stopTimer += dt;

            if (state.stopTimer >= 0.0f)
                state.stopTimer = 0.0f;

            return;
        }

        state.stopTimer += dt;

        if (state.stopTimer >= a90::STOP_DURATION) {
            removeA90UI(&state);

            state.phase = a90::Phase::Inactive;
            state.stopTimer = 0.0f;

            state.triggerTimer = randomFloat(
                a90::TRIGGER_DELAY_MIN,
                a90::TRIGGER_DELAY_MAX
            );
        }
    }

    void destroyPlayer(
        PlayerObject* player,
        GameObject* object
    ) {
        auto& state = m_fields->state;

        state.dead = true;
        state.levelActive = false;

        removeA90UI(&state);

        state.phase = a90::Phase::Inactive;
        state.stopTimer = 0.0f;

        PlayLayer::destroyPlayer(
            player,
            object
        );
    }

    void resetLevel() {
        auto& state = m_fields->state;

        removeA90UI(&state);

        state.phase = a90::Phase::Inactive;
        state.stopTimer = 0.0f;
        state.levelActive = true;
        state.dead = false;

        state.triggerTimer = randomFloat(
            a90::TRIGGER_DELAY_MIN,
            a90::TRIGGER_DELAY_MAX
        );

        PlayLayer::resetLevel();
    }

    void levelComplete() {
        auto& state = m_fields->state;

        state.levelActive = false;

        removeA90UI(&state);

        state.phase = a90::Phase::Inactive;
        state.stopTimer = 0.0f;

        PlayLayer::levelComplete();
    }

    void onQuit() {
        auto& state = m_fields->state;

        state.levelActive = false;
        state.dead = true;

        removeA90UI(&state);

        state.phase = a90::Phase::Inactive;
        state.stopTimer = 0.0f;

        PlayLayer::onQuit();
    }

    ~A90PlayLayer() {
        removeA90UI(&m_fields->state);
    }
};

class $modify(A90InputLayer, GJBaseGameLayer) {
public:
    void handleButton(
        bool down,
        int button,
        bool player1
    ) {
        auto* playLayer =
            typeinfo_cast<A90PlayLayer*>(this);

        if (playLayer) {
            auto& state =
                playLayer->m_fields->state;

            if (down &&
                state.levelActive &&
                !state.dead &&
                state.phase == a90::Phase::Stop &&
                state.stopTimer >= 0.0f &&
                isGameplayInput(button)) {

                PlayerObject* player =
                    player1
                        ? playLayer->m_player1
                        : playLayer->m_player2;

                state.dead = true;
                state.levelActive = false;
                state.phase = a90::Phase::Inactive;
                state.stopTimer = 0.0f;

                showA90Jumpscare(&state);

                if (player) {
                    playLayer->destroyPlayer(
                        player,
                        nullptr
                    );
                }

                return;
            }
        }

        GJBaseGameLayer::handleButton(
            down,
            button,
            player1
        );
    }
};
