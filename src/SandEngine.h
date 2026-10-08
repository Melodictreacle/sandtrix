#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <utility>
#include <QString>
#include "Config.h"
#include "Tetromino.h"

struct EngineTickEvent {
    bool cleared = false;
    bool isTidalWave = false;
    int grains = 0;
    int bands = 0;
    std::vector<std::pair<int, int>> coords; // (y, x)
    uint8_t color = 0;
    int combo = 0;
    int points = 0;
    bool landed = false;
    bool landedWater = false;
    bool landedAcid = false;
    bool landedBomb = false;
    std::vector<std::pair<int, int>> acidCorrodedCoords;
    bool isClassicTetris = false;
    int classicLines = 0;
    bool isMystery = false;
    bool shapeShifted = false;
    char newShape = 0;
    bool bombDetonated = false;
    int bombX = 0;
    int bombY = 0;
    std::vector<std::pair<int, int>> bombCraterCoords;
    bool earthquake = false;
    bool windGust = false;
    int windDir = 0;
    bool confusionTriggered = false;
};

class SandEngine {
public:
    explicit SandEngine(int numColors = 4, bool enableElements = false, int startLevel = 1, GameMode mode = GameMode::Sandtrix);

    void reset();
    void setPaletteColorsCount(int numColors);
    void applySettings(const GameSettings& settings, int numColors);

    bool spawnPiece();
    bool holdCurrentPiece();
    bool movePiece(int dx);
    bool rotatePiece(int direction);
    bool hardDrop(int& outX, int& outY, std::vector<std::pair<int, int>>& outImpactCoords);
    void lockActivePiece(EngineTickEvent* outEvent = nullptr);

    bool updatePhysics(std::vector<std::pair<int, int>>* outAcidCorroded = nullptr);
    EngineTickEvent tick(int dtMs);
    void triggerEarthquake();
    bool detonateBombAt(int cx, int cy, int radius, std::vector<std::pair<int, int>>& outCrater);

    // Getters
    const std::vector<uint8_t>& getGrid() const { return m_grid; }
    Tetromino* getActivePiece() const { return m_activePiece.get(); }
    Tetromino* getHoldPiece() const { return m_holdPiece.get(); }
    const std::vector<std::unique_ptr<Tetromino>>& getNextQueue() const { return m_nextQueue; }
    int getActiveGhostY() const;

    bool isClassicTetris() const { return m_gameMode == GameMode::ClassicTetris || m_gameMode == GameMode::MysteryClassic; }
    bool isMystery() const { return m_gameMode == GameMode::Mystery || m_gameMode == GameMode::MysteryClassic; }
    GameMode getGameMode() const { return m_gameMode; }

    int getScore() const { return m_score; }
    int getHighScore() const { return m_highScore; }
    int getLevel() const { return m_level; }
    int getLinesCleared() const { return m_linesCleared; }
    int getCombo() const { return m_combo; }
    bool isGameOver() const { return m_gameOver; }
    bool isPaused() const { return m_paused; }
    void setPaused(bool paused) { m_paused = paused; }
    void togglePaused() { m_paused = !m_paused; }

    void setSoftDropping(bool dropping) { m_isSoftDropping = dropping; }
    bool isSoftDropping() const { return m_isSoftDropping; }

private:
    void refillNextQueue();
    int loadHighScore();
    void saveHighScore();
    std::pair<int, int> checkLineClears(std::vector<std::pair<int, int>>& outCoords, uint8_t& outLastColor);
    int checkClassicLineClears(std::vector<std::pair<int, int>>& outCoords);

    int m_numColors;
    int m_startLevel;
    bool m_enableElements;
    GameMode m_gameMode;
    std::vector<uint8_t> m_grid; // size BOARD_HEIGHT * BOARD_WIDTH

    int m_score;
    int m_highScore;
    int m_level;
    int m_linesCleared;
    int m_combo;
    bool m_gameOver;
    bool m_paused;

    BagRandomizer m_randomizer;
    std::unique_ptr<Tetromino> m_activePiece;
    std::unique_ptr<Tetromino> m_holdPiece;
    bool m_canHold;
    std::vector<std::unique_ptr<Tetromino>> m_nextQueue;

    int m_gravityTimer;
    int m_lockTimer;
    bool m_isSoftDropping;
    bool m_isSandMoving;
    bool m_lastDropHadClear;

    // Mystery Mode Variables
    int m_pieceShiftTriggerY;
    bool m_pieceHasShifted;
    bool m_pieceEligibleForShift;
    int m_mysteryCalamityTimer;
};
