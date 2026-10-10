#include "SandEngine.h"
#include <random>
#include <algorithm>
#include <queue>
#include <cmath>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QCoreApplication>

namespace {
const std::vector<std::pair<int, int>> BFS_NEIGHBORS = {
    {-1, 0},
    { 0, -1}, { 0, 1},
    { 1, 0}
};

int getGravityInterval(int level) {
    static const int intervals[] = { 45, 40, 35, 30, 25, 20, 16, 13, 10, 8 };
    int idx = std::max(1, std::min(10, level)) - 1;
    return intervals[idx];
}

int getClassicGravityInterval(int level) {
    static const int intervals[] = { 500, 440, 380, 320, 260, 210, 160, 120, 90, 70 };
    int idx = std::max(1, std::min(10, level)) - 1;
    return intervals[idx];
}
} // namespace

SandEngine::SandEngine(int numColors, bool enableElements, int startLevel, GameMode mode)
    : m_numColors(numColors),
      m_startLevel(startLevel),
      m_enableElements(enableElements && (mode == GameMode::Sandtrix)),
      m_gameMode(mode),
      m_grid(BOARD_HEIGHT * BOARD_WIDTH, 0),
      m_score(0),
      m_highScore(0),
      m_level(startLevel),
      m_linesCleared(0),
      m_combo(0),
      m_gameOver(false),
      m_paused(true),
      m_randomizer(numColors, enableElements && (mode == GameMode::Sandtrix), mode == GameMode::Mystery || mode == GameMode::MysteryClassic),
      m_canHold(true),
      m_gravityTimer(0),
      m_lockTimer(0),
      m_isSoftDropping(false),
      m_isSandMoving(false),
      m_lastDropHadClear(false),
      m_pieceShiftTriggerY(0),
      m_pieceHasShifted(false),
      m_pieceEligibleForShift(false),
      m_mysteryCalamityTimer(0),
      m_lineGoal(0),
      m_missionCompleted(false)
{
    m_highScore = loadHighScore();
    refillNextQueue();
    spawnPiece();
}

int SandEngine::loadHighScore() {
    QFile file("highscores.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject()) {
            int hs = doc.object().value("high_score").toInt(0);
            if (hs < 0 || hs > 999999999) {
                hs = 0;
            }
            return hs;
        }
    }
    return 0;
}

void SandEngine::saveHighScore() {
    QFile file("highscores.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        int safeHigh = std::max(0, std::min(999999999, m_highScore));
        obj["high_score"] = safeHigh;
        file.write(QJsonDocument(obj).toJson());
    }
}

void SandEngine::setPaletteColorsCount(int numColors) {
    m_numColors = numColors;
    m_randomizer.setNumColors(numColors);
    reset();
}

void SandEngine::applySettings(const GameSettings& settings, int numColors) {
    m_numColors = numColors;
    m_startLevel = settings.startingLevel;
    m_gameMode = settings.gameMode;
    m_enableElements = settings.enableElements && (settings.gameMode == GameMode::Sandtrix);
    m_lineGoal = settings.lineGoal;
    reset();
}

int SandEngine::getActiveGhostY() const {
    if (!m_activePiece) return 0;
    int step = isClassicTetris() ? MINO_SIZE : 1;
    return m_activePiece->getGhostY(m_grid, step);
}

void SandEngine::reset() {
    std::fill(m_grid.begin(), m_grid.end(), 0);
    m_score = 0;
    m_level = m_startLevel;
    m_linesCleared = 0;
    m_combo = 0;
    m_gameOver = false;
    m_paused = false;
    m_missionCompleted = false;
    m_holdPiece.reset();
    m_canHold = true;
    m_gravityTimer = 0;
    m_lockTimer = 0;
    m_isSoftDropping = false;
    m_isSandMoving = false;
    m_lastDropHadClear = false;

    m_pieceShiftTriggerY = 0;
    m_pieceHasShifted = false;
    m_pieceEligibleForShift = false;
    m_mysteryCalamityTimer = 0;

    m_stats = RunStats();
    m_stats.lineGoal = m_lineGoal;

    m_lastFrozenCoords.clear();
    m_lastMeltedCoords.clear();
    m_lastSteamCoords.clear();
    m_lastToxicCoords.clear();
    m_lastBurnedCoords.clear();
    m_lastExtinguishedCoords.clear();

    m_randomizer = BagRandomizer(m_numColors, m_enableElements && (m_gameMode == GameMode::Sandtrix), isMystery());
    m_nextQueue.clear();
    refillNextQueue();
    spawnPiece();
}

void SandEngine::refillNextQueue() {
    while (m_nextQueue.size() < 4) {
        m_nextQueue.push_back(m_randomizer.nextPiece());
    }
}

bool SandEngine::spawnPiece() {
    refillNextQueue();
    m_activePiece = std::move(m_nextQueue.front());
    m_nextQueue.erase(m_nextQueue.begin());
    m_lockTimer = 0;
    m_gravityTimer = 0;

    m_pieceHasShifted = false;
    if (isMystery()) {
        static std::random_device rd;
        static std::mt19937 rng(rd());
        std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
        m_pieceEligibleForShift = (chanceDist(rng) < 0.55f);
        if (isClassicTetris()) {
            std::uniform_int_distribution<int> rowDist(3, 7);
            m_pieceShiftTriggerY = 4 + rowDist(rng) * MINO_SIZE;
        } else {
            std::uniform_int_distribution<int> yDist(28, 62);
            m_pieceShiftTriggerY = yDist(rng);
        }
    } else {
        m_pieceEligibleForShift = false;
    }

    int startY = isClassicTetris() ? 4 : 0;
    m_activePiece->setY(startY);

    if (m_activePiece->collides(m_grid)) {
        m_gameOver = true;
        if (m_score > m_highScore) {
            m_highScore = m_score;
            saveHighScore();
        }
        return false;
    }
    return true;
}

bool SandEngine::detonateBombAt(int cx, int cy, int radius, std::vector<std::pair<int, int>>& outCrater) {
    outCrater.clear();
    if (isClassicTetris()) {
        int centerMinoX = cx / MINO_SIZE;
        int centerMinoY = (cy - 4) / MINO_SIZE;
        for (int dmy = -1; dmy <= 1; ++dmy) {
            for (int dmx = -1; dmx <= 1; ++dmx) {
                int mx = centerMinoX + dmx;
                int my = centerMinoY + dmy;
                if (mx >= 0 && mx < 10 && my >= 0 && my < 17) {
                    int startX = mx * MINO_SIZE;
                    int startY = 4 + my * MINO_SIZE;
                    for (int gy = 0; gy < MINO_SIZE; ++gy) {
                        for (int gx = 0; gx < MINO_SIZE; ++gx) {
                            int idx = (startY + gy) * BOARD_WIDTH + (startX + gx);
                            if (m_grid[idx] > 0) {
                                outCrater.push_back({startY + gy, startX + gx});
                                m_grid[idx] = 0;
                            }
                        }
                    }
                }
            }
        }
        return !outCrater.empty();
    }

    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dy * dy <= r2) {
                int gx = cx + dx;
                int gy = cy + dy;
                if (gx >= 0 && gx < BOARD_WIDTH && gy >= 0 && gy < BOARD_HEIGHT) {
                    int idx = gy * BOARD_WIDTH + gx;
                    if (m_grid[idx] > 0) {
                        outCrater.push_back({gy, gx});
                        m_grid[idx] = 0;
                    }
                }
            }
        }
    }
    m_isSandMoving = true;
    return !outCrater.empty();
}

void SandEngine::triggerEarthquake() {
    static std::random_device rd;
    static std::mt19937 rng(rd());
    std::uniform_int_distribution<int> colDist(0, BOARD_WIDTH - 1);

    for (int i = 0; i < 70; ++i) {
        int x = colDist(rng);
        for (int y = BOARD_HEIGHT - 3; y >= 10; --y) {
            uint8_t c = m_grid[y * BOARD_WIDTH + x];
            if (c > 0 && m_grid[(y + 1) * BOARD_WIDTH + x] == 0) {
                m_grid[(y + 1) * BOARD_WIDTH + x] = c;
                m_grid[y * BOARD_WIDTH + x] = 0;
            }
        }
    }
    m_isSandMoving = true;
}

bool SandEngine::holdCurrentPiece() {
    if (!m_canHold || !m_activePiece || m_gameOver || m_paused) {
        return false;
    }

    char shape = m_activePiece->getShape();
    uint8_t col = m_activePiece->getColorIdx();
    int startY = isClassicTetris() ? 4 : 0;

    if (!m_holdPiece) {
        m_holdPiece = std::make_unique<Tetromino>(shape, col);
        spawnPiece();
    } else {
        char heldShape = m_holdPiece->getShape();
        uint8_t heldCol = m_holdPiece->getColorIdx();

        m_holdPiece = std::make_unique<Tetromino>(shape, col);
        m_activePiece = std::make_unique<Tetromino>(heldShape, heldCol);
        m_activePiece->setY(startY);
    }

    m_canHold = false;
    m_lockTimer = 0;
    m_gravityTimer = 0;
    return true;
}

bool SandEngine::movePiece(int dx) {
    if (m_activePiece && !m_gameOver && !m_paused) {
        if (m_activePiece->tryMove(dx, 0, m_grid)) {
            m_lockTimer = 0;
            return true;
        }
        // If full step is blocked by an obstacle closer than dx, slide flush against it
        int dir = (dx > 0) ? 1 : -1;
        int absDx = std::abs(dx);
        for (int step = absDx - 1; step >= 1; --step) {
            if (m_activePiece->tryMove(step * dir, 0, m_grid)) {
                m_lockTimer = 0;
                return true;
            }
        }
        // Allow step-up (1 or 2 grains) over minor sand grain roughness on the floor
        for (int dy = -1; dy >= -2; --dy) {
            if (m_activePiece->tryMove(dx, dy, m_grid)) {
                m_lockTimer = 0;
                return true;
            }
        }
    }
    return false;
}

bool SandEngine::rotatePiece(int direction) {
    if (m_activePiece && !m_gameOver && !m_paused) {
        bool rot = m_activePiece->tryRotate(direction, m_grid);
        if (rot) {
            m_lockTimer = 0;
        }
        return rot;
    }
    return false;
}

bool SandEngine::hardDrop(int& outX, int& outY, std::vector<std::pair<int, int>>& outImpactCoords) {
    if (!m_activePiece || m_gameOver || m_paused) {
        return false;
    }

    int ghostY = getActiveGhostY();
    m_activePiece->setY(ghostY);
    outX = m_activePiece->getX();
    outY = ghostY;

    // When ice block moves downwards, existing water in contact converts into ice
    if (m_activePiece->isIce()) {
        static const int dOffsets[6][2] = {{1, 0}, {2, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}};
        for (const auto& g : m_activePiece->getOccupiedGrains()) {
            for (const auto& d : dOffsets) {
                int ny = g.y + d[0];
                int nx = g.x + d[1];
                if (ny >= 0 && ny < BOARD_HEIGHT && nx >= 0 && nx < BOARD_WIDTH) {
                    if (m_grid[ny * BOARD_WIDTH + nx] == MATERIAL_WATER) {
                        m_grid[ny * BOARD_WIDTH + nx] = MATERIAL_ICE;
                        m_lastFrozenCoords.push_back({ny, nx});
                    }
                }
            }
        }
    }

    outImpactCoords.clear();
    for (const auto& g : m_activePiece->getOccupiedGrains()) {
        outImpactCoords.push_back({g.y, g.x});
    }

    // In Mystery mode, hard drop has a 30% chance to cause an earthquake tremor!
    if (isMystery() && !isClassicTetris()) {
        static std::random_device rd;
        static std::mt19937 rng(rd());
        std::uniform_real_distribution<float> dropTremor(0.0f, 1.0f);
        if (dropTremor(rng) < 0.30f) {
            triggerEarthquake();
        }
    }

    lockActivePiece();
    return true;
}

void SandEngine::lockActivePiece(EngineTickEvent* outEvent) {
    if (!m_activePiece) return;

    bool isWater = m_activePiece->isWater();
    bool isAcid = m_activePiece->isAcid();
    bool isBomb = m_activePiece->isBomb();
    bool isIce = m_activePiece->isIce();
    bool isFire = m_activePiece->isFire();
    if (outEvent) {
        outEvent->landedWater = isWater;
        outEvent->landedAcid = isAcid;
        outEvent->landedBomb = isBomb;
        outEvent->landedIce = isIce;
        outEvent->landedFire = isFire;
    }

    if (isIce) {
        static const int dOffsets[6][2] = {{1, 0}, {2, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}};
        for (const auto& g : m_activePiece->getOccupiedGrains()) {
            for (const auto& d : dOffsets) {
                int ny = g.y + d[0];
                int nx = g.x + d[1];
                if (ny >= 0 && ny < BOARD_HEIGHT && nx >= 0 && nx < BOARD_WIDTH) {
                    if (m_grid[ny * BOARD_WIDTH + nx] == MATERIAL_WATER) {
                        m_grid[ny * BOARD_WIDTH + nx] = MATERIAL_ICE;
                        m_lastFrozenCoords.push_back({ny, nx});
                    }
                }
            }
        }
    }

    bool hitWater = false;
    bool hitIce = false;
    bool hitAcid = false;
    bool hitSand = false;
    std::vector<std::pair<int, int>> contactAcid;
    std::vector<std::pair<int, int>> contactSand;

    if (isFire) {
        // Check if fire piece landed on or touches other materials
        for (const auto& g : m_activePiece->getOccupiedGrains()) {
            static const int dCoords[7][2] = {{1, 0}, {0, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}};
            for (const auto& d : dCoords) {
                int ny = g.y + d[0];
                int nx = g.x + d[1];
                if (ny >= 0 && ny < BOARD_HEIGHT && nx >= 0 && nx < BOARD_WIDTH) {
                    uint8_t val = m_grid[ny * BOARD_WIDTH + nx];
                    if (val == MATERIAL_WATER) {
                        hitWater = true;
                    } else if (val == MATERIAL_ICE) {
                        hitIce = true;
                    } else if (val == MATERIAL_ACID) {
                        hitAcid = true;
                        contactAcid.push_back({ny, nx});
                    } else if ((val >= 1 && val <= 10) || val == MATERIAL_ASH) {
                        hitSand = true;
                        contactSand.push_back({ny, nx});
                    }
                }
            }
        }
    }

    if (isFire && hitWater) {
        // 0. Fire on Water: When fire falls down upon water, fire is totally destroyed!
        for (const auto& g : m_activePiece->getOccupiedGrains()) {
            if (g.y >= 0 && g.y < BOARD_HEIGHT && g.x >= 0 && g.x < BOARD_WIDTH) {
                m_lastExtinguishedCoords.push_back({g.y, g.x});
            }
        }
        // Fire is completely erased/extinguished: none of its grains are placed on the grid
    } else if (isFire && hitIce) {
        // 1. Fire on Ice: Converts ice to water AND fire to steam (steam disappears after few seconds)
        for (int i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; ++i) {
            if (m_grid[i] == MATERIAL_ICE) {
                m_grid[i] = MATERIAL_WATER;
                m_lastMeltedCoords.push_back({i / BOARD_WIDTH, i % BOARD_WIDTH});
            }
        }
        for (const auto& g : m_activePiece->getOccupiedGrains()) {
            if (g.y >= 0 && g.y < BOARD_HEIGHT && g.x >= 0 && g.x < BOARD_WIDTH) {
                m_grid[g.y * BOARD_WIDTH + g.x] = MATERIAL_STEAM;
                m_lastSteamCoords.push_back({g.y, g.x});
            }
        }
    } else if (isFire && hitAcid) {
        // 2. Fire on Acid: Creates green toxic cloud (disappears after few seconds)
        for (const auto& pt : contactAcid) {
            m_grid[pt.first * BOARD_WIDTH + pt.second] = MATERIAL_TOXIC_GAS;
            m_lastToxicCoords.push_back(pt);
        }
        for (const auto& g : m_activePiece->getOccupiedGrains()) {
            if (g.y >= 0 && g.y < BOARD_HEIGHT && g.x >= 0 && g.x < BOARD_WIDTH) {
                m_grid[g.y * BOARD_WIDTH + g.x] = MATERIAL_TOXIC_GAS;
                m_lastToxicCoords.push_back({g.y, g.x});
            }
        }
    } else if (isFire && hitSand) {
        // 3. Fire on Sand: Portion of sands burn and convert to ash; fire piece burns entirely to ash (no red pixels remain!)
        static std::random_device rd;
        static std::mt19937 rng(rd());
        std::uniform_real_distribution<float> burnDist(0.0f, 1.0f);

        for (const auto& pt : contactSand) {
            if (burnDist(rng) < 0.60f) {
                m_grid[pt.first * BOARD_WIDTH + pt.second] = MATERIAL_ASH;
                m_lastBurnedCoords.push_back(pt);
            }
        }
        // Fire piece settles entirely as ash residue (100% ash, 0% red fire)
        for (const auto& g : m_activePiece->getOccupiedGrains()) {
            if (g.y >= 0 && g.y < BOARD_HEIGHT && g.x >= 0 && g.x < BOARD_WIDTH) {
                m_grid[g.y * BOARD_WIDTH + g.x] = MATERIAL_ASH;
                m_lastBurnedCoords.push_back({g.y, g.x});
            }
        }
    } else if (isBomb) {
        int cx = m_activePiece->getX() + m_activePiece->getMaskWidth() / 2;
        int cy = m_activePiece->getY() + m_activePiece->getMaskHeight() / 2;
        std::vector<std::pair<int, int>> crater;
        detonateBombAt(cx, cy, 14, crater);
        if (outEvent) {
            outEvent->bombDetonated = true;
            outEvent->bombX = cx;
            outEvent->bombY = cy;
            outEvent->bombCraterCoords = crater;
        }
        int64_t rawPoints = static_cast<int64_t>(crater.size()) * 15 * m_level;
        int points = static_cast<int>(std::max<int64_t>(0, std::min<int64_t>(1000000, rawPoints)));
        int64_t newScore = static_cast<int64_t>(m_score) + points;
        m_score = static_cast<int>(std::max<int64_t>(0, std::min<int64_t>(999999999, newScore)));
        if (m_score > m_highScore) {
            m_highScore = m_score;
            saveHighScore();
        }
    } else {
        for (const auto& g : m_activePiece->getOccupiedGrains()) {
            if (g.y >= 0 && g.y < BOARD_HEIGHT && g.x >= 0 && g.x < BOARD_WIDTH) {
                uint8_t cur = m_grid[g.y * BOARD_WIDTH + g.x];
                if (cur == 0 || cur == MATERIAL_STEAM || cur == MATERIAL_TOXIC_GAS) {
                    m_grid[g.y * BOARD_WIDTH + g.x] = g.color;
                }
            }
        }
    }

    m_stats.piecesPlaced++;
    m_activePiece.reset();
    m_canHold = true;
    m_lockTimer = 0;
    m_isSoftDropping = false;
    m_isSandMoving = !isClassicTetris();

    // Check danger ceiling breach (ignoring rising atmospheric gases)
    int dangerLimit = isClassicTetris() ? (4 + MINO_SIZE) : DANGER_ROW;
    for (int r = 0; r < dangerLimit; ++r) {
        for (int c = 0; c < BOARD_WIDTH; ++c) {
            uint8_t val = m_grid[r * BOARD_WIDTH + c];
            if (val > 0 && val != MATERIAL_STEAM && val != MATERIAL_TOXIC_GAS) {
                m_gameOver = true;
                if (m_score > m_highScore) {
                    m_highScore = m_score;
                    saveHighScore();
                }
                return;
            }
        }
    }

    spawnPiece();
}

bool SandEngine::updatePhysics(std::vector<std::pair<int, int>>* outAcidCorroded) {
    bool moved = false;
    static std::random_device rd;
    static std::mt19937 g(rd());
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    std::vector<int> colXs;
    colXs.reserve(BOARD_WIDTH);

    // ==================== PASS 1: RISING GASES (STEAM & TOXIC CLOUD) ====================
    // Process top-down so gas rises at most 1 grain per sub-step
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        colXs.clear();
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            uint8_t c = m_grid[y * BOARD_WIDTH + x];
            if (c == MATERIAL_STEAM || c == MATERIAL_TOXIC_GAS) {
                colXs.push_back(x);
            }
        }
        if (colXs.empty()) continue;
        std::shuffle(colXs.begin(), colXs.end(), g);

        for (int x : colXs) {
            uint8_t gmat = m_grid[y * BOARD_WIDTH + x];
            if (gmat != MATERIAL_STEAM && gmat != MATERIAL_TOXIC_GAS) continue;

            // Spontaneous dissipation over a few seconds (~2-3s half life)
            float dissRate = (gmat == MATERIAL_STEAM) ? 0.010f : 0.012f;
            if (prob(g) < dissRate) {
                m_grid[y * BOARD_WIDTH + x] = 0;
                moved = true;
                continue;
            }

            // Dissipate into open air at ceiling
            if (y == 0) {
                m_grid[y * BOARD_WIDTH + x] = 0;
                moved = true;
                continue;
            }

            // Buoyant upward motion
            if (m_grid[(y - 1) * BOARD_WIDTH + x] == 0) {
                m_grid[(y - 1) * BOARD_WIDTH + x] = gmat;
                m_grid[y * BOARD_WIDTH + x] = 0;
                moved = true;
            } else {
                bool ulEmpty = (x > 0 && m_grid[(y - 1) * BOARD_WIDTH + (x - 1)] == 0);
                bool urEmpty = (x < BOARD_WIDTH - 1 && m_grid[(y - 1) * BOARD_WIDTH + (x + 1)] == 0);
                if (ulEmpty && urEmpty) {
                    int nx = (std::uniform_int_distribution<int>(0, 1)(g) == 0) ? (x - 1) : (x + 1);
                    m_grid[(y - 1) * BOARD_WIDTH + nx] = gmat;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    moved = true;
                } else if (ulEmpty) {
                    m_grid[(y - 1) * BOARD_WIDTH + (x - 1)] = gmat;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    moved = true;
                } else if (urEmpty) {
                    m_grid[(y - 1) * BOARD_WIDTH + (x + 1)] = gmat;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    moved = true;
                } else {
                    bool lEmpty = (x > 0 && m_grid[y * BOARD_WIDTH + (x - 1)] == 0);
                    bool rEmpty = (x < BOARD_WIDTH - 1 && m_grid[y * BOARD_WIDTH + (x + 1)] == 0);
                    if ((lEmpty || rEmpty) && prob(g) < 0.25f) {
                        int nx = (lEmpty && (!rEmpty || prob(g) < 0.5f)) ? (x - 1) : (x + 1);
                        m_grid[y * BOARD_WIDTH + nx] = gmat;
                        m_grid[y * BOARD_WIDTH + x] = 0;
                        moved = true;
                    }
                }
            }
        }
    }

    // ==================== PASS 2: FALLING / SETTLING MATTER ====================
    // Process from bottom row up for gravity
    for (int y = BOARD_HEIGHT - 2; y >= 0; --y) {
        colXs.clear();
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            uint8_t c = m_grid[y * BOARD_WIDTH + x];
            if (c > 0 && c != MATERIAL_STEAM && c != MATERIAL_TOXIC_GAS) {
                colXs.push_back(x);
            }
        }

        if (colXs.empty()) continue;
        std::shuffle(colXs.begin(), colXs.end(), g);

        for (int x : colXs) {
            uint8_t mat = m_grid[y * BOARD_WIDTH + x];
            if (mat == 0 || mat == MATERIAL_STEAM || mat == MATERIAL_TOXIC_GAS) continue;

            // ==================== 1. WATER (FLUID) DYNAMICS ====================
            if (mat == MATERIAL_WATER) {
                if (m_grid[(y + 1) * BOARD_WIDTH + x] == 0) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = MATERIAL_WATER;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    moved = true;
                } else {
                    bool dlEmpty = (x > 0 && m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == 0);
                    bool drEmpty = (x < BOARD_WIDTH - 1 && m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == 0);

                    if (dlEmpty && drEmpty) {
                        int nx = (std::uniform_int_distribution<int>(0, 1)(g) == 0) ? (x - 1) : (x + 1);
                        m_grid[(y + 1) * BOARD_WIDTH + nx] = MATERIAL_WATER;
                        m_grid[y * BOARD_WIDTH + x] = 0;
                        moved = true;
                    } else if (dlEmpty) {
                        m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] = MATERIAL_WATER;
                        m_grid[y * BOARD_WIDTH + x] = 0;
                        moved = true;
                    } else if (drEmpty) {
                        m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] = MATERIAL_WATER;
                        m_grid[y * BOARD_WIDTH + x] = 0;
                        moved = true;
                    } else {
                        bool leftEmpty = (x > 0 && m_grid[y * BOARD_WIDTH + (x - 1)] == 0);
                        bool rightEmpty = (x < BOARD_WIDTH - 1 && m_grid[y * BOARD_WIDTH + (x + 1)] == 0);

                        if (leftEmpty && rightEmpty) {
                            int nx = (std::uniform_int_distribution<int>(0, 1)(g) == 0) ? (x - 1) : (x + 1);
                            m_grid[y * BOARD_WIDTH + nx] = MATERIAL_WATER;
                            m_grid[y * BOARD_WIDTH + x] = 0;
                            moved = true;
                        } else if (leftEmpty) {
                            m_grid[y * BOARD_WIDTH + (x - 1)] = MATERIAL_WATER;
                            m_grid[y * BOARD_WIDTH + x] = 0;
                            moved = true;
                        } else if (rightEmpty) {
                            m_grid[y * BOARD_WIDTH + (x + 1)] = MATERIAL_WATER;
                            m_grid[y * BOARD_WIDTH + x] = 0;
                            moved = true;
                        }
                    }
                }
            }
            // ==================== 2. ACID (CORROSION) DYNAMICS ====================
            else if (mat == MATERIAL_ACID) {
                uint8_t below = m_grid[(y + 1) * BOARD_WIDTH + x];
                if (below == 0) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = MATERIAL_ACID;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    moved = true;
                } else if (below != MATERIAL_ACID && below != MATERIAL_WATER && below != MATERIAL_FIRE && below != MATERIAL_ICE && below != MATERIAL_TOXIC_GAS) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = 0;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    if (outAcidCorroded) outAcidCorroded->push_back({y + 1, x});
                    moved = true;
                } else {
                    bool dlEmpty = (x > 0 && m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == 0);
                    bool drEmpty = (x < BOARD_WIDTH - 1 && m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == 0);
                    if ((dlEmpty || drEmpty) && prob(g) < 0.25f) {
                        int nx = (dlEmpty && (!drEmpty || prob(g) < 0.5f)) ? (x - 1) : (x + 1);
                        m_grid[(y + 1) * BOARD_WIDTH + nx] = MATERIAL_ACID;
                        m_grid[y * BOARD_WIDTH + x] = 0;
                        moved = true;
                    }
                }
            }
            // ==================== 3. ICE DYNAMICS ====================
            else if (mat == MATERIAL_ICE) {
                uint8_t below = m_grid[(y + 1) * BOARD_WIDTH + x];
                if (below == MATERIAL_WATER) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = MATERIAL_ICE;
                    m_lastFrozenCoords.push_back({y + 1, x});
                    moved = true;
                } else if (below == 0) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = MATERIAL_ICE;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    moved = true;
                } else {
                    bool dlFree = (x > 0 && (m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == 0 || m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == MATERIAL_WATER));
                    bool drFree = (x < BOARD_WIDTH - 1 && (m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == 0 || m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == MATERIAL_WATER));
                    if ((dlFree || drFree) && prob(g) < 0.12f) {
                        int nx = (dlFree && (!drFree || prob(g) < 0.5f)) ? (x - 1) : (x + 1);
                        uint8_t targetVal = m_grid[(y + 1) * BOARD_WIDTH + nx];
                        if (targetVal == MATERIAL_WATER) {
                            m_grid[(y + 1) * BOARD_WIDTH + nx] = MATERIAL_ICE;
                            m_lastFrozenCoords.push_back({y + 1, nx});
                        } else {
                            m_grid[(y + 1) * BOARD_WIDTH + nx] = MATERIAL_ICE;
                            m_grid[y * BOARD_WIDTH + x] = 0;
                        }
                        moved = true;
                    }
                }
            }
            // ==================== 4. FIRE DYNAMICS ====================
            else if (mat == MATERIAL_FIRE) {
                uint8_t below = m_grid[(y + 1) * BOARD_WIDTH + x];
                // A. Fire touching Water: fire is totally destroyed!
                if (below == MATERIAL_WATER) {
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    m_lastExtinguishedCoords.push_back({y, x});
                    moved = true;
                }
                // B. Fire touching Ice: melts ice to water AND fire turns to steam!
                else if (below == MATERIAL_ICE) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = MATERIAL_WATER;
                    m_grid[y * BOARD_WIDTH + x] = MATERIAL_STEAM;
                    m_lastMeltedCoords.push_back({y + 1, x});
                    m_lastSteamCoords.push_back({y, x});
                    moved = true;
                }
                // C. Fire touching Acid: creates green toxic cloud!
                else if (below == MATERIAL_ACID) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = MATERIAL_TOXIC_GAS;
                    m_grid[y * BOARD_WIDTH + x] = MATERIAL_TOXIC_GAS;
                    m_lastToxicCoords.push_back({y + 1, x});
                    m_lastToxicCoords.push_back({y, x});
                    moved = true;
                }
                // D. Fire touching Sand or Ash: burns sand into ash, fire itself converts to ash! (No red pixels!)
                else if ((below >= 1 && below <= 10) || below == MATERIAL_ASH) {
                    if (below >= 1 && below <= 10) {
                        m_grid[(y + 1) * BOARD_WIDTH + x] = MATERIAL_ASH;
                        m_lastBurnedCoords.push_back({y + 1, x});
                    }
                    m_grid[y * BOARD_WIDTH + x] = MATERIAL_ASH;
                    m_lastBurnedCoords.push_back({y, x});
                    moved = true;
                }
                // E. Empty below
                else if (below == 0) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = MATERIAL_FIRE;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    moved = true;
                } else {
                    bool dlWater = (x > 0 && m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == MATERIAL_WATER);
                    bool drWater = (x < BOARD_WIDTH - 1 && m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == MATERIAL_WATER);
                    bool lWater = (x > 0 && m_grid[y * BOARD_WIDTH + (x - 1)] == MATERIAL_WATER);
                    bool rWater = (x < BOARD_WIDTH - 1 && m_grid[y * BOARD_WIDTH + (x + 1)] == MATERIAL_WATER);
                    bool dlIce = (x > 0 && m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == MATERIAL_ICE);
                    bool drIce = (x < BOARD_WIDTH - 1 && m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == MATERIAL_ICE);
                    bool dlAcid = (x > 0 && m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == MATERIAL_ACID);
                    bool drAcid = (x < BOARD_WIDTH - 1 && m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == MATERIAL_ACID);
                    bool dlSand = (x > 0 && ((m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] >= 1 && m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] <= 10) || m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == MATERIAL_ASH));
                    bool drSand = (x < BOARD_WIDTH - 1 && ((m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] >= 1 && m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] <= 10) || m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == MATERIAL_ASH));

                    if (dlWater || drWater || lWater || rWater) {
                        m_grid[y * BOARD_WIDTH + x] = 0;
                        m_lastExtinguishedCoords.push_back({y, x});
                        moved = true;
                    } else if (dlIce) {
                        m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] = MATERIAL_WATER;
                        m_grid[y * BOARD_WIDTH + x] = MATERIAL_STEAM;
                        m_lastMeltedCoords.push_back({y + 1, x - 1});
                        m_lastSteamCoords.push_back({y, x});
                        moved = true;
                    } else if (drIce) {
                        m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] = MATERIAL_WATER;
                        m_grid[y * BOARD_WIDTH + x] = MATERIAL_STEAM;
                        m_lastMeltedCoords.push_back({y + 1, x + 1});
                        m_lastSteamCoords.push_back({y, x});
                        moved = true;
                    } else if (dlAcid) {
                        m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] = MATERIAL_TOXIC_GAS;
                        m_grid[y * BOARD_WIDTH + x] = MATERIAL_TOXIC_GAS;
                        m_lastToxicCoords.push_back({y + 1, x - 1});
                        m_lastToxicCoords.push_back({y, x});
                        moved = true;
                    } else if (drAcid) {
                        m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] = MATERIAL_TOXIC_GAS;
                        m_grid[y * BOARD_WIDTH + x] = MATERIAL_TOXIC_GAS;
                        m_lastToxicCoords.push_back({y + 1, x + 1});
                        m_lastToxicCoords.push_back({y, x});
                        moved = true;
                    } else if (dlSand || drSand) {
                        m_grid[y * BOARD_WIDTH + x] = MATERIAL_ASH;
                        m_lastBurnedCoords.push_back({y, x});
                        moved = true;
                    } else {
                        bool dlEmpty = (x > 0 && m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == 0);
                        bool drEmpty = (x < BOARD_WIDTH - 1 && m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == 0);
                        if (dlEmpty || drEmpty) {
                            int nx = (std::uniform_int_distribution<int>(0, 1)(g) == 0) ? (x - 1) : (x + 1);
                            if ((nx == x - 1 && !dlEmpty) || (nx == x + 1 && !drEmpty)) nx = (nx == x - 1) ? (x + 1) : (x - 1);
                            m_grid[(y + 1) * BOARD_WIDTH + nx] = MATERIAL_FIRE;
                            m_grid[y * BOARD_WIDTH + x] = 0;
                            moved = true;
                        }
                    }
                }
            }
            // ==================== 5. ASH DYNAMICS (BURNT SAND POWDER) ====================
            else if (mat == MATERIAL_ASH) {
                uint8_t below = m_grid[(y + 1) * BOARD_WIDTH + x];
                if (below == 0) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = MATERIAL_ASH;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    moved = true;
                } else if (below == MATERIAL_WATER) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = MATERIAL_ASH;
                    m_grid[y * BOARD_WIDTH + x] = MATERIAL_WATER;
                    moved = true;
                } else {
                    bool dlFree = (x > 0 && (m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == 0 || m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == MATERIAL_WATER));
                    bool drFree = (x < BOARD_WIDTH - 1 && (m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == 0 || m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == MATERIAL_WATER));
                    if ((dlFree || drFree) && prob(g) < 0.10f) {
                        int nx = (dlFree && (!drFree || prob(g) < 0.5f)) ? (x - 1) : (x + 1);
                        uint8_t targetVal = m_grid[(y + 1) * BOARD_WIDTH + nx];
                        m_grid[(y + 1) * BOARD_WIDTH + nx] = MATERIAL_ASH;
                        m_grid[y * BOARD_WIDTH + x] = (targetVal == MATERIAL_WATER) ? MATERIAL_WATER : 0;
                        moved = true;
                    }
                }
            }
            // ==================== 6. NORMAL SAND DYNAMICS (BUOYANCY INTERACTION) ====================
            else {
                uint8_t below = m_grid[(y + 1) * BOARD_WIDTH + x];
                
                if (below == 0) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = mat;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    moved = true;
                } else if (below == MATERIAL_WATER) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = mat;
                    m_grid[y * BOARD_WIDTH + x] = MATERIAL_WATER;
                    moved = true;
                } else {
                    bool dlOk = (x > 0 && (m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == 0 || m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == MATERIAL_WATER));
                    bool drOk = (x < BOARD_WIDTH - 1 && (m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == 0 || m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == MATERIAL_WATER));

                    if ((dlOk || drOk) && prob(g) < 0.08f) {
                        int nx = (dlOk && (!drOk || prob(g) < 0.5f)) ? (x - 1) : (x + 1);
                        uint8_t targetVal = m_grid[(y + 1) * BOARD_WIDTH + nx];
                        m_grid[(y + 1) * BOARD_WIDTH + nx] = mat;
                        m_grid[y * BOARD_WIDTH + x] = (targetVal == MATERIAL_WATER) ? MATERIAL_WATER : 0;
                        moved = true;
                    }
                }
            }
        }
    }
    return moved;
}

std::pair<int, int> SandEngine::checkLineClears(std::vector<std::pair<int, int>>& outCoords, uint8_t& outLastColor) {
    outCoords.clear();
    int bandsCount = 0;
    outLastColor = 0;

    std::vector<bool> visited(BOARD_HEIGHT * BOARD_WIDTH, false);

    std::vector<int> colorsToCheck;
    for (int c = 1; c <= m_numColors; ++c) colorsToCheck.push_back(c);
    if (m_enableElements) {
        colorsToCheck.push_back(MATERIAL_WATER);
        colorsToCheck.push_back(MATERIAL_ICE);
        colorsToCheck.push_back(MATERIAL_FIRE);
        colorsToCheck.push_back(MATERIAL_ASH);
    }

    for (int color : colorsToCheck) {
        std::vector<int> startRows;
        for (int y = 0; y < BOARD_HEIGHT; ++y) {
            if (m_grid[y * BOARD_WIDTH + 0] == color) {
                startRows.push_back(y);
            }
        }

        if (startRows.empty()) continue;

        std::fill(visited.begin(), visited.end(), false);

        for (int sy : startRows) {
            if (visited[sy * BOARD_WIDTH + 0]) continue;

            std::queue<std::pair<int, int>> q;
            q.push({sy, 0});
            visited[sy * BOARD_WIDTH + 0] = true;

            std::vector<std::pair<int, int>> component;
            component.push_back({sy, 0});
            bool touchesRightWall = false;

            while (!q.empty()) {
                auto [cy, cx] = q.front();
                q.pop();

                if (cx == BOARD_WIDTH - 1) {
                    touchesRightWall = true;
                }

                for (const auto& [dy, dx] : BFS_NEIGHBORS) {
                    int ny = cy + dy;
                    int nx = cx + dx;
                    if (ny >= 0 && ny < BOARD_HEIGHT && nx >= 0 && nx < BOARD_WIDTH) {
                        int idx = ny * BOARD_WIDTH + nx;
                        if (!visited[idx] && m_grid[idx] == color) {
                            visited[idx] = true;
                            q.push({ny, nx});
                            component.push_back({ny, nx});
                        }
                    }
                }
            }

            if (touchesRightWall) {
                bandsCount++;
                outLastColor = static_cast<uint8_t>(color);
                for (const auto& pt : component) {
                    outCoords.push_back(pt);
                }
            }
        }
    }

    if (!outCoords.empty()) {
        std::sort(outCoords.begin(), outCoords.end());
        outCoords.erase(std::unique(outCoords.begin(), outCoords.end()), outCoords.end());

        for (const auto& [cy, cx] : outCoords) {
            m_grid[cy * BOARD_WIDTH + cx] = 0;
        }
        return {static_cast<int>(outCoords.size()), bandsCount};
    }

    return {0, 0};
}

int SandEngine::checkClassicLineClears(std::vector<std::pair<int, int>>& outCoords) {
    outCoords.clear();
    std::vector<int> fullRows;

    // Check each of the 17 mino rows (row 0 to 16, where mino row R starts at y = 4 + R * 8)
    for (int r = 0; r < 17; ++r) {
        int yStart = 4 + r * MINO_SIZE;
        bool full = true;
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            if (m_grid[yStart * BOARD_WIDTH + x] == 0) {
                full = false;
                break;
            }
        }
        if (full) {
            fullRows.push_back(r);
        }
    }

    if (fullRows.empty()) return 0;

    // Record all grain coords of cleared rows for particles
    for (int r : fullRows) {
        int yStart = 4 + r * MINO_SIZE;
        for (int gy = 0; gy < MINO_SIZE; ++gy) {
            for (int x = 0; x < BOARD_WIDTH; ++x) {
                outCoords.push_back({yStart + gy, x});
            }
        }
    }

    // Collapse board: for each row from bottom (16) up to 0, shift it down by the count of cleared rows below it
    std::vector<uint8_t> newGrid(BOARD_HEIGHT * BOARD_WIDTH, 0);

    for (int r = 16; r >= 0; --r) {
        if (std::find(fullRows.begin(), fullRows.end(), r) != fullRows.end()) {
            continue; // Skip cleared row
        }

        int drop = 0;
        for (int cr : fullRows) {
            if (cr > r) drop++;
        }

        int targetR = r + drop;
        if (targetR < 17) {
            int srcY = 4 + r * MINO_SIZE;
            int dstY = 4 + targetR * MINO_SIZE;
            for (int gy = 0; gy < MINO_SIZE; ++gy) {
                for (int x = 0; x < BOARD_WIDTH; ++x) {
                    newGrid[(dstY + gy) * BOARD_WIDTH + x] = m_grid[(srcY + gy) * BOARD_WIDTH + x];
                }
            }
        }
    }

    m_grid = std::move(newGrid);
    return static_cast<int>(fullRows.size());
}

EngineTickEvent SandEngine::tick(int dtMs) {
    EngineTickEvent event;
    if (m_gameOver || m_missionCompleted || m_paused) return event;

    m_stats.timeElapsedMs += dtMs;
    m_stats.score = m_score;
    m_stats.linesCleared = m_linesCleared;
    m_stats.maxCombo = std::max(m_stats.maxCombo, m_combo);

    event.isClassicTetris = isClassicTetris();
    event.isMystery = isMystery();

    if (isMystery()) {
        // A. Mid-Air Shape Shifting!
        if (m_activePiece && m_pieceEligibleForShift && !m_pieceHasShifted) {
            if (m_activePiece->getY() >= m_pieceShiftTriggerY) {
                static std::random_device rd;
                static std::mt19937 rng(rd());
                static const std::vector<char> possibleShapes = {'I', 'O', 'T', 'S', 'Z', 'J', 'L', 'D', '+'};
                char curShape = m_activePiece->getShape();
                std::vector<char> choices;
                for (char s : possibleShapes) {
                    if (s != curShape) choices.push_back(s);
                }
                std::uniform_int_distribution<size_t> d(0, choices.size() - 1);
                char targetShape = choices[d(rng)];

                if (m_activePiece->morphTo(targetShape, m_grid)) {
                    m_pieceHasShifted = true;
                    event.shapeShifted = true;
                    event.newShape = targetShape;
                }
            }
        }

        // B. Calamity Events Timer (Earthquake, Wind Gust, Control Confusion)
        m_mysteryCalamityTimer += dtMs;
        if (m_mysteryCalamityTimer >= 16000) {
            m_mysteryCalamityTimer = 0;
            static std::random_device rd;
            static std::mt19937 rng(rd());
            std::uniform_int_distribution<int> cRoll(0, 2);
            int roll = cRoll(rng);

            if (roll == 0) {
                if (!isClassicTetris()) {
                    triggerEarthquake();
                }
                event.earthquake = true;
            } else if (roll == 1) {
                if (m_activePiece) {
                    std::uniform_int_distribution<int> dirRoll(0, 1);
                    int dir = (dirRoll(rng) == 0) ? -1 : 1;
                    if (m_activePiece->tryMove(dir * MINO_SIZE, 0, m_grid)) {
                        event.windGust = true;
                        event.windDir = dir;
                    }
                }
            } else {
                event.confusionTriggered = true;
            }
        }
    }

    if (isClassicTetris()) {
        // ==================== CLASSIC TETRIS MODE ====================
        m_isSandMoving = false;

        // Check for complete horizontal row clears
        std::vector<std::pair<int, int>> clearedCoords;
        int linesCount = checkClassicLineClears(clearedCoords);

        if (linesCount > 0) {
            static const int lineScores[] = { 0, 100, 300, 500, 800 };
            int basePoints = (linesCount <= 4) ? lineScores[linesCount] : (800 + (linesCount - 4) * 200);
            int64_t rawPoints = static_cast<int64_t>(basePoints) * m_level;
            int points = static_cast<int>(std::max<int64_t>(0, std::min<int64_t>(1000000, rawPoints)));

            int64_t newScore = static_cast<int64_t>(m_score) + points;
            m_score = static_cast<int>(std::max<int64_t>(0, std::min<int64_t>(999999999, newScore)));

            if (m_score > m_highScore) {
                m_highScore = m_score;
                saveHighScore();
            }

            m_linesCleared += linesCount;
            m_stats.linesCleared = m_linesCleared;
            m_stats.score = m_score;

            if (linesCount == 1) m_stats.singles++;
            else if (linesCount == 2) m_stats.doubles++;
            else if (linesCount == 3) m_stats.triples++;
            else if (linesCount >= 4) m_stats.tetrises++;

            // Check Mission Line Goal
            if (m_lineGoal > 0 && m_linesCleared >= m_lineGoal && !m_missionCompleted) {
                m_missionCompleted = true;
                m_stats.victory = true;
                event.missionCompleted = true;
            }

            m_level = m_startLevel + (m_linesCleared / 10);
            m_lastDropHadClear = true;

            event.cleared = true;
            event.classicLines = linesCount;
            event.bands = linesCount;
            event.coords = clearedCoords;
            event.points = points;
            event.combo = linesCount;
        }

        // Active piece gravity & lock (1 mino step = 8 grains)
        if (m_activePiece) {
            int gravityInterval = m_isSoftDropping ? 30 : getClassicGravityInterval(m_level);
            m_gravityTimer += dtMs;

            if (m_gravityTimer >= gravityInterval) {
                m_gravityTimer = 0;
                if (m_activePiece->tryMove(0, MINO_SIZE, m_grid)) {
                    m_lockTimer = 0;
                } else {
                    m_lockTimer += gravityInterval;
                    if (m_lockTimer >= 450) {
                        lockActivePiece(&event);
                        event.landed = true;
                    }
                }
            }
        }

        return event;
    }

    // ==================== SANDTRIX MODE ====================

    // 1. Update sand cellular automata physics sub-steps
    bool anyMoved = false;
    for (int s = 0; s < PHYSICS_SUBSTEPS; ++s) {
        if (updatePhysics(&event.acidCorrodedCoords)) {
            anyMoved = true;
        }
    }
    m_isSandMoving = anyMoved;

    // 2. Check for edge-to-edge line clears
    std::vector<std::pair<int, int>> clearedCoords;
    uint8_t clearedCol = 0;
    auto [grainsCount, bandsCount] = checkLineClears(clearedCoords, clearedCol);

    if (bandsCount > 0) {
        m_combo++;
        double comboExponent = std::min(12.0, static_cast<double>(m_combo - 1));
        double comboMult = std::pow(COMBO_MULTIPLIER_BASE, comboExponent);
        comboMult = std::min(40.0, comboMult);

        int bonusPerBand = (clearedCol == MATERIAL_WATER) ? (POINTS_PER_BAND * 2) : POINTS_PER_BAND;
        int64_t rawPoints = static_cast<int64_t>((grainsCount * POINTS_PER_GRAIN + bandsCount * bonusPerBand) * comboMult * m_level);
        int points = static_cast<int>(std::max<int64_t>(0, std::min<int64_t>(5000000, rawPoints)));

        int64_t newScore = static_cast<int64_t>(m_score) + points;
        m_score = static_cast<int>(std::max<int64_t>(0, std::min<int64_t>(999999999, newScore)));

        if (m_score > m_highScore) {
            m_highScore = m_score;
            saveHighScore();
        }

        m_linesCleared += bandsCount;
        m_stats.linesCleared = m_linesCleared;
        m_stats.score = m_score;
        m_stats.maxCombo = std::max(m_stats.maxCombo, m_combo);

        // Check Mission Line Goal
        if (m_lineGoal > 0 && m_linesCleared >= m_lineGoal && !m_missionCompleted) {
            m_missionCompleted = true;
            m_stats.victory = true;
            event.missionCompleted = true;
        }

        m_level = m_startLevel + (m_linesCleared / 5);
        m_lastDropHadClear = true;

        event.cleared = true;
        event.isTidalWave = (clearedCol == MATERIAL_WATER);
        event.isIceClear = (clearedCol == MATERIAL_ICE);
        event.isFireClear = (clearedCol == MATERIAL_FIRE);
        event.isAshClear = (clearedCol == MATERIAL_ASH);
        event.grains = grainsCount;
        event.bands = bandsCount;
        event.coords = clearedCoords;
        event.color = clearedCol;
        event.combo = m_combo;
        event.points = points;
    } else if (!anyMoved && !m_isSandMoving) {
        if (!m_lastDropHadClear) {
            m_combo = 0;
        }
        m_lastDropHadClear = false;
    }

    // 3. Active Tetromino Gravity & Lock
    if (m_activePiece) {
        int gravityInterval = m_isSoftDropping ? SOFT_DROP_GRAVITY_MS : getGravityInterval(m_level);
        m_gravityTimer += dtMs;
        int steps = 0;
        const int maxSteps = 12;

        while (m_gravityTimer >= gravityInterval && steps < maxSteps) {
            m_gravityTimer -= gravityInterval;
            steps++;
            if (m_activePiece->tryMove(0, 1, m_grid)) {
                m_lockTimer = 0;
                // When ice block moves downwards, existing water in contact converts into ice
                if (m_activePiece->isIce()) {
                    static const int dOffsets[6][2] = {{1, 0}, {2, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}};
                    for (const auto& g : m_activePiece->getOccupiedGrains()) {
                        for (const auto& d : dOffsets) {
                            int ny = g.y + d[0];
                            int nx = g.x + d[1];
                            if (ny >= 0 && ny < BOARD_HEIGHT && nx >= 0 && nx < BOARD_WIDTH) {
                                if (m_grid[ny * BOARD_WIDTH + nx] == MATERIAL_WATER) {
                                    m_grid[ny * BOARD_WIDTH + nx] = MATERIAL_ICE;
                                    m_lastFrozenCoords.push_back({ny, nx});
                                }
                            }
                        }
                    }
                }
            } else {
                m_lockTimer += gravityInterval;
                // If fire piece hits water directly below, extinguish instantly
                bool fireHitWater = false;
                if (m_activePiece->isFire()) {
                    for (const auto& g : m_activePiece->getOccupiedGrains()) {
                        int ny = g.y + 1;
                        int nx = g.x;
                        if (ny >= 0 && ny < BOARD_HEIGHT && nx >= 0 && nx < BOARD_WIDTH) {
                            if (m_grid[ny * BOARD_WIDTH + nx] == MATERIAL_WATER) {
                                fireHitWater = true;
                                break;
                            }
                        }
                    }
                }
                if (m_lockTimer >= LOCK_DELAY_MS || m_isSoftDropping || fireHitWater) {
                    lockActivePiece(&event);
                    event.landed = true;
                }
                break;
            }
        }
        if (m_gravityTimer > gravityInterval * 3) {
            m_gravityTimer = 0;
        }
    }

    if (!m_lastFrozenCoords.empty()) {
        event.waterFrozen = true;
        event.frozenCoords = m_lastFrozenCoords;
        m_lastFrozenCoords.clear();
    }
    if (!m_lastMeltedCoords.empty()) {
        event.iceMelted = true;
        event.meltedCoords = m_lastMeltedCoords;
        m_lastMeltedCoords.clear();
    }
    if (!m_lastSteamCoords.empty()) {
        event.steamCreated = true;
        event.steamCoords = m_lastSteamCoords;
        m_lastSteamCoords.clear();
    }
    if (!m_lastToxicCoords.empty()) {
        event.toxicCloudCreated = true;
        event.toxicCoords = m_lastToxicCoords;
        m_lastToxicCoords.clear();
    }
    if (!m_lastBurnedCoords.empty()) {
        event.sandBurned = true;
        event.burnedCoords = m_lastBurnedCoords;
        m_lastBurnedCoords.clear();
    }
    if (!m_lastExtinguishedCoords.empty()) {
        event.fireExtinguished = true;
        event.extinguishedCoords = m_lastExtinguishedCoords;
        m_lastExtinguishedCoords.clear();
    }

    return event;
}
