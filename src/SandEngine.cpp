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
    {-1, -1}, {-1, 0}, {-1, 1},
    { 0, -1},          { 0, 1},
    { 1, -1}, { 1, 0}, { 1, 1}
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
      m_paused(false),
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
        return m_activePiece->tryMove(dx, 0, m_grid);
    }
    return false;
}

bool SandEngine::rotatePiece(int direction) {
    if (m_activePiece && !m_gameOver && !m_paused) {
        return m_activePiece->tryRotate(direction, m_grid);
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
    if (outEvent) {
        outEvent->landedWater = isWater;
        outEvent->landedAcid = isAcid;
        outEvent->landedBomb = isBomb;
    }

    if (isBomb) {
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
                m_grid[g.y * BOARD_WIDTH + g.x] = g.color;
            }
        }
    }

    m_stats.piecesPlaced++;
    m_activePiece.reset();
    m_canHold = true;
    m_lockTimer = 0;
    m_isSandMoving = !isClassicTetris();

    // Check danger ceiling breach
    int dangerLimit = isClassicTetris() ? (4 + MINO_SIZE) : DANGER_ROW;
    for (int r = 0; r < dangerLimit; ++r) {
        for (int c = 0; c < BOARD_WIDTH; ++c) {
            if (m_grid[r * BOARD_WIDTH + c] > 0) {
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

    std::vector<int> colXs;
    colXs.reserve(BOARD_WIDTH);

    // Process from bottom row up
    for (int y = BOARD_HEIGHT - 2; y >= 0; --y) {
        colXs.clear();
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            if (m_grid[y * BOARD_WIDTH + x] > 0) {
                colXs.push_back(x);
            }
        }

        if (colXs.empty()) continue;

        std::shuffle(colXs.begin(), colXs.end(), g);

        for (int x : colXs) {
            uint8_t mat = m_grid[y * BOARD_WIDTH + x];
            if (mat == 0) continue;

            // ==================== 1. WATER (FLUID) DYNAMICS ====================
            if (mat == MATERIAL_WATER) {
                // A. Straight Down
                if (m_grid[(y + 1) * BOARD_WIDTH + x] == 0) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = MATERIAL_WATER;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    moved = true;
                }
                // B. Down Diagonals
                else {
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
                    }
                    // C. Lateral Flow (Fluid seeks flat surface)
                    else {
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
                // A. Straight Down
                uint8_t below = m_grid[(y + 1) * BOARD_WIDTH + x];
                if (below == 0) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = MATERIAL_ACID;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    moved = true;
                } else if (below != MATERIAL_ACID && below != 0) {
                    m_grid[(y + 1) * BOARD_WIDTH + x] = 0;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    if (outAcidCorroded) outAcidCorroded->push_back({y + 1, x});
                    moved = true;
                } else {
                    bool dlEmpty = (x > 0 && m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == 0);
                    bool drEmpty = (x < BOARD_WIDTH - 1 && m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == 0);
                    if (dlEmpty && drEmpty) {
                        int nx = (std::uniform_int_distribution<int>(0, 1)(g) == 0) ? (x - 1) : (x + 1);
                        m_grid[(y + 1) * BOARD_WIDTH + nx] = MATERIAL_ACID;
                        m_grid[y * BOARD_WIDTH + x] = 0;
                        moved = true;
                    } else if (dlEmpty) {
                        m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] = MATERIAL_ACID;
                        m_grid[y * BOARD_WIDTH + x] = 0;
                        moved = true;
                    } else if (drEmpty) {
                        m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] = MATERIAL_ACID;
                        m_grid[y * BOARD_WIDTH + x] = 0;
                        moved = true;
                    }
                }
            }
            // ==================== 3. SAND DYNAMICS (BUOYANCY INTERACTION) ====================
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

                    if (dlOk && drOk) {
                        int nx = (std::uniform_int_distribution<int>(0, 1)(g) == 0) ? (x - 1) : (x + 1);
                        uint8_t targetVal = m_grid[(y + 1) * BOARD_WIDTH + nx];
                        m_grid[(y + 1) * BOARD_WIDTH + nx] = mat;
                        m_grid[y * BOARD_WIDTH + x] = (targetVal == MATERIAL_WATER) ? MATERIAL_WATER : 0;
                        moved = true;
                    } else if (dlOk) {
                        uint8_t targetVal = m_grid[(y + 1) * BOARD_WIDTH + (x - 1)];
                        m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] = mat;
                        m_grid[y * BOARD_WIDTH + x] = (targetVal == MATERIAL_WATER) ? MATERIAL_WATER : 0;
                        moved = true;
                    } else if (drOk) {
                        uint8_t targetVal = m_grid[(y + 1) * BOARD_WIDTH + (x + 1)];
                        m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] = mat;
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
                    if (m_lockTimer >= LOCK_DELAY_MS || m_isSoftDropping) {
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

        if (m_gravityTimer >= gravityInterval) {
            m_gravityTimer = 0;
            if (m_activePiece->tryMove(0, 1, m_grid)) {
                m_lockTimer = 0;
            } else {
                m_lockTimer += gravityInterval;
                if (m_lockTimer >= LOCK_DELAY_MS || m_isSoftDropping) {
                    lockActivePiece(&event);
                    event.landed = true;
                }
            }
        }
    }

    return event;
}
