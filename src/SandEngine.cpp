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
} // namespace

SandEngine::SandEngine(int numColors)
    : m_numColors(numColors),
      m_grid(BOARD_HEIGHT * BOARD_WIDTH, 0),
      m_score(0),
      m_highScore(0),
      m_level(1),
      m_linesCleared(0),
      m_combo(0),
      m_gameOver(false),
      m_paused(false),
      m_randomizer(numColors),
      m_canHold(true),
      m_gravityTimer(0),
      m_lockTimer(0),
      m_isSoftDropping(false),
      m_isSandMoving(false),
      m_lastDropHadClear(false)
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
            return doc.object().value("high_score").toInt(0);
        }
    }
    return 0;
}

void SandEngine::saveHighScore() {
    QFile file("highscores.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        obj["high_score"] = m_highScore;
        file.write(QJsonDocument(obj).toJson());
    }
}

void SandEngine::setPaletteColorsCount(int numColors) {
    m_numColors = numColors;
    m_randomizer.setNumColors(numColors);
    reset();
}

void SandEngine::reset() {
    std::fill(m_grid.begin(), m_grid.end(), 0);
    m_score = 0;
    m_level = 1;
    m_linesCleared = 0;
    m_combo = 0;
    m_gameOver = false;
    m_paused = false;
    m_holdPiece.reset();
    m_canHold = true;
    m_gravityTimer = 0;
    m_lockTimer = 0;
    m_isSoftDropping = false;
    m_isSandMoving = false;
    m_lastDropHadClear = false;

    m_randomizer = BagRandomizer(m_numColors);
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

bool SandEngine::holdCurrentPiece() {
    if (!m_canHold || !m_activePiece || m_gameOver || m_paused) {
        return false;
    }

    char shape = m_activePiece->getShape();
    uint8_t col = m_activePiece->getColorIdx();

    if (!m_holdPiece) {
        m_holdPiece = std::make_unique<Tetromino>(shape, col);
        spawnPiece();
    } else {
        char heldShape = m_holdPiece->getShape();
        uint8_t heldCol = m_holdPiece->getColorIdx();

        m_holdPiece = std::make_unique<Tetromino>(shape, col);
        m_activePiece = std::make_unique<Tetromino>(heldShape, heldCol);
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

    int ghostY = m_activePiece->getGhostY(m_grid);
    m_activePiece->setY(ghostY);
    outX = m_activePiece->getX();
    outY = ghostY;

    outImpactCoords.clear();
    for (const auto& g : m_activePiece->getOccupiedGrains()) {
        outImpactCoords.push_back({g.y, g.x});
    }

    lockActivePiece();
    return true;
}

void SandEngine::lockActivePiece() {
    if (!m_activePiece) return;

    for (const auto& g : m_activePiece->getOccupiedGrains()) {
        if (g.y >= 0 && g.y < BOARD_HEIGHT && g.x >= 0 && g.x < BOARD_WIDTH) {
            m_grid[g.y * BOARD_WIDTH + g.x] = g.color;
        }
    }

    m_activePiece.reset();
    m_canHold = true;
    m_lockTimer = 0;
    m_isSandMoving = true;

    // Check danger ceiling breach
    for (int r = 0; r < DANGER_ROW; ++r) {
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

bool SandEngine::updatePhysics() {
    bool moved = false;
    static std::random_device rd;
    static std::mt19937 g(rd());

    std::vector<int> sandXs;
    sandXs.reserve(BOARD_WIDTH);

    // Process from bottom row up
    for (int y = BOARD_HEIGHT - 2; y >= 0; --y) {
        sandXs.clear();
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            if (m_grid[y * BOARD_WIDTH + x] > 0) {
                sandXs.push_back(x);
            }
        }

        if (sandXs.empty()) continue;

        std::shuffle(sandXs.begin(), sandXs.end(), g);

        for (int x : sandXs) {
            uint8_t color = m_grid[y * BOARD_WIDTH + x];
            if (color == 0) continue;

            // 1. Straight down
            if (m_grid[(y + 1) * BOARD_WIDTH + x] == 0) {
                m_grid[(y + 1) * BOARD_WIDTH + x] = color;
                m_grid[y * BOARD_WIDTH + x] = 0;
                moved = true;
            } else {
                // 2. Diagonals
                bool leftOk = (x > 0 && m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] == 0);
                bool rightOk = (x < BOARD_WIDTH - 1 && m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] == 0);

                if (leftOk && rightOk) {
                    int nx = (std::uniform_int_distribution<int>(0, 1)(g) == 0) ? (x - 1) : (x + 1);
                    m_grid[(y + 1) * BOARD_WIDTH + nx] = color;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    moved = true;
                } else if (leftOk) {
                    m_grid[(y + 1) * BOARD_WIDTH + (x - 1)] = color;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    moved = true;
                } else if (rightOk) {
                    m_grid[(y + 1) * BOARD_WIDTH + (x + 1)] = color;
                    m_grid[y * BOARD_WIDTH + x] = 0;
                    moved = true;
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

    for (int color = 1; color <= m_numColors; ++color) {
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
        // Deduplicate
        std::sort(outCoords.begin(), outCoords.end());
        outCoords.erase(std::unique(outCoords.begin(), outCoords.end()), outCoords.end());

        // Clear cells from grid
        for (const auto& [cy, cx] : outCoords) {
            m_grid[cy * BOARD_WIDTH + cx] = 0;
        }
        return {static_cast<int>(outCoords.size()), bandsCount};
    }

    return {0, 0};
}

EngineTickEvent SandEngine::tick(int dtMs) {
    EngineTickEvent event;
    if (m_gameOver || m_paused) return event;

    // 1. Update sand cellular automata physics sub-steps
    bool anyMoved = false;
    for (int s = 0; s < PHYSICS_SUBSTEPS; ++s) {
        if (updatePhysics()) {
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
        double comboMult = std::pow(COMBO_MULTIPLIER_BASE, m_combo - 1);
        int points = static_cast<int>((grainsCount * POINTS_PER_GRAIN + bandsCount * POINTS_PER_BAND) * comboMult * m_level);
        m_score += points;
        if (m_score > m_highScore) {
            m_highScore = m_score;
            saveHighScore();
        }

        m_linesCleared += bandsCount;
        m_level = 1 + (m_linesCleared / 5);
        m_lastDropHadClear = true;

        event.cleared = true;
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
                    lockActivePiece();
                    event.landed = true;
                }
            }
        }
    }

    return event;
}
