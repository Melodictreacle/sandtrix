#include "Tetromino.h"
#include <random>
#include <algorithm>

namespace {
// Wall kicks offsets in grains to test during rotation
const std::vector<std::pair<int, int>> WALL_KICK_TESTS = {
    {0, 0},
    {MINO_SIZE, 0},
    {-MINO_SIZE, 0},
    {2 * MINO_SIZE, 0},
    {-2 * MINO_SIZE, 0},
    {0, -MINO_SIZE},
    {MINO_SIZE, -MINO_SIZE},
    {-MINO_SIZE, -MINO_SIZE}
};

std::vector<std::vector<uint8_t>> getMinoMatrix(char shape, int rot) {
    rot = (rot % 4 + 4) % 4;
    switch (shape) {
        case 'I': {
            std::vector<std::vector<uint8_t>> m(4, std::vector<uint8_t>(4, 0));
            if (rot == 0) { m[1][0] = m[1][1] = m[1][2] = m[1][3] = 1; }
            else if (rot == 1) { m[0][2] = m[1][2] = m[2][2] = m[3][2] = 1; }
            else if (rot == 2) { m[2][0] = m[2][1] = m[2][2] = m[2][3] = 1; }
            else { m[0][1] = m[1][1] = m[2][1] = m[3][1] = 1; }
            return m;
        }
        case 'O': {
            return {{1, 1}, {1, 1}};
        }
        case 'T': {
            std::vector<std::vector<uint8_t>> m(3, std::vector<uint8_t>(3, 0));
            if (rot == 0) { m[0][1] = m[1][0] = m[1][1] = m[1][2] = 1; }
            else if (rot == 1) { m[0][1] = m[1][1] = m[1][2] = m[2][1] = 1; }
            else if (rot == 2) { m[1][0] = m[1][1] = m[1][2] = m[2][1] = 1; }
            else { m[0][1] = m[1][0] = m[1][1] = m[2][1] = 1; }
            return m;
        }
        case 'S': {
            std::vector<std::vector<uint8_t>> m(3, std::vector<uint8_t>(3, 0));
            if (rot == 0) { m[0][1] = m[0][2] = m[1][0] = m[1][1] = 1; }
            else if (rot == 1) { m[0][1] = m[1][1] = m[1][2] = m[2][2] = 1; }
            else if (rot == 2) { m[1][1] = m[1][2] = m[2][0] = m[2][1] = 1; }
            else { m[0][0] = m[1][0] = m[1][1] = m[2][1] = 1; }
            return m;
        }
        case 'Z': {
            std::vector<std::vector<uint8_t>> m(3, std::vector<uint8_t>(3, 0));
            if (rot == 0) { m[0][0] = m[0][1] = m[1][1] = m[1][2] = 1; }
            else if (rot == 1) { m[0][2] = m[1][1] = m[1][2] = m[2][1] = 1; }
            else if (rot == 2) { m[1][0] = m[1][1] = m[2][1] = m[2][2] = 1; }
            else { m[0][1] = m[1][0] = m[1][1] = m[2][0] = 1; }
            return m;
        }
        case 'J': {
            std::vector<std::vector<uint8_t>> m(3, std::vector<uint8_t>(3, 0));
            if (rot == 0) { m[0][0] = m[1][0] = m[1][1] = m[1][2] = 1; }
            else if (rot == 1) { m[0][1] = m[0][2] = m[1][1] = m[2][1] = 1; }
            else if (rot == 2) { m[1][0] = m[1][1] = m[1][2] = m[2][2] = 1; }
            else { m[0][1] = m[1][1] = m[2][0] = m[2][1] = 1; }
            return m;
        }
        case 'L': {
            std::vector<std::vector<uint8_t>> m(3, std::vector<uint8_t>(3, 0));
            if (rot == 0) { m[0][2] = m[1][0] = m[1][1] = m[1][2] = 1; }
            else if (rot == 1) { m[0][1] = m[1][1] = m[2][1] = m[2][2] = 1; }
            else if (rot == 2) { m[1][0] = m[1][1] = m[1][2] = m[2][0] = 1; }
            else { m[0][0] = m[0][1] = m[1][1] = m[2][1] = 1; }
            return m;
        }
        default:
            return {{1, 1}, {1, 1}};
    }
}
} // namespace

Tetromino::Tetromino(char shape, uint8_t colorIdx)
    : m_shape(shape), m_colorIdx(colorIdx), m_rotation(0), m_y(0)
{
    m_matrixSize = (shape == 'I') ? 4 : ((shape == 'O') ? 2 : 3);
    m_maskWidth = m_matrixSize * MINO_SIZE;
    m_maskHeight = m_matrixSize * MINO_SIZE;
    int boardMinos = BOARD_WIDTH / MINO_SIZE;
    int spawnCol = (boardMinos - m_matrixSize) / 2;
    m_x = spawnCol * MINO_SIZE;
    updateGrainMask();
}

QString Tetromino::getElementName() const {
    if (isWater()) return "WATER";
    if (isAcid()) return "ACID";
    return "";
}

void Tetromino::updateGrainMask() {
    auto minoM = getMinoMatrix(m_shape, m_rotation);
    int mRows = static_cast<int>(minoM.size());
    int mCols = static_cast<int>(minoM[0].size());

    m_maskHeight = mRows * MINO_SIZE;
    m_maskWidth = mCols * MINO_SIZE;
    m_grainMask.assign(m_maskHeight * m_maskWidth, false);

    for (int r = 0; r < mRows; ++r) {
        for (int c = 0; c < mCols; ++c) {
            if (minoM[r][c]) {
                for (int gy = 0; gy < MINO_SIZE; ++gy) {
                    for (int gx = 0; gx < MINO_SIZE; ++gx) {
                        int py = r * MINO_SIZE + gy;
                        int px = c * MINO_SIZE + gx;
                        m_grainMask[py * m_maskWidth + px] = true;
                    }
                }
            }
        }
    }
}

bool Tetromino::collides(const std::vector<uint8_t>& grid, int offsetX, int offsetY, int testRotation) const {
    int testRot = (testRotation >= 0) ? testRotation : m_rotation;
    auto minoM = getMinoMatrix(m_shape, testRot);
    int mRows = static_cast<int>(minoM.size());
    int mCols = static_cast<int>(minoM[0].size());

    int targetX = m_x + offsetX;
    int targetY = m_y + offsetY;

    for (int r = 0; r < mRows; ++r) {
        for (int c = 0; c < mCols; ++c) {
            if (!minoM[r][c]) continue;

            for (int gy = 0; gy < MINO_SIZE; ++gy) {
                for (int gx = 0; gx < MINO_SIZE; ++gx) {
                    int absX = targetX + c * MINO_SIZE + gx;
                    int absY = targetY + r * MINO_SIZE + gy;

                    // Boundaries
                    if (absX < 0 || absX >= BOARD_WIDTH || absY >= BOARD_HEIGHT) {
                        return true;
                    }

                    // Existing sand or water/acid
                    if (absY >= 0) {
                        if (grid[absY * BOARD_WIDTH + absX] > 0) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool Tetromino::tryMove(int dx, int dy, const std::vector<uint8_t>& grid) {
    if (!collides(grid, dx, dy)) {
        m_x += dx;
        m_y += dy;
        return true;
    }
    return false;
}

bool Tetromino::tryRotate(int direction, const std::vector<uint8_t>& grid) {
    int newRot = (m_rotation + direction % 4 + 4) % 4;
    for (const auto& kick : WALL_KICK_TESTS) {
        if (!collides(grid, kick.first, kick.second, newRot)) {
            m_rotation = newRot;
            m_x += kick.first;
            m_y += kick.second;
            updateGrainMask();
            return true;
        }
    }
    return false;
}

int Tetromino::getGhostY(const std::vector<uint8_t>& grid) const {
    int ghostY = m_y;
    while (!collides(grid, 0, ghostY - m_y + 1)) {
        ghostY += 1;
    }
    return ghostY;
}

std::vector<GrainCoord> Tetromino::getOccupiedGrains() const {
    std::vector<GrainCoord> coords;
    for (int gy = 0; gy < m_maskHeight; ++gy) {
        for (int gx = 0; gx < m_maskWidth; ++gx) {
            if (m_grainMask[gy * m_maskWidth + gx]) {
                coords.push_back({m_y + gy, m_x + gx, m_colorIdx});
            }
        }
    }
    return coords;
}

// ------------------- BagRandomizer -------------------

BagRandomizer::BagRandomizer(int numColors, bool enableElements)
    : m_numColors(numColors), m_enableElements(enableElements), m_piecesSpawned(0)
{
    refillBag();
}

void BagRandomizer::refillBag() {
    m_bag = {'I', 'O', 'T', 'S', 'Z', 'J', 'L'};
    static std::random_device rd;
    static std::mt19937 g(rd());
    std::shuffle(m_bag.begin(), m_bag.end(), g);
}

uint8_t BagRandomizer::pickColor(size_t sequenceIndex) {
    static std::random_device rd;
    static std::mt19937 rng(rd());

    if (m_enableElements) {
        std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
        float roll = chanceDist(rng);

        if (roll < 0.15f) {
            return MATERIAL_WATER; // 🌊 Water block!
        } else if (roll < 0.22f) {
            return MATERIAL_ACID;  // ☣ Acid block!
        }
    }

    std::uniform_int_distribution<int> dist(1, m_numColors);
    return static_cast<uint8_t>(dist(rng));
}

std::unique_ptr<Tetromino> BagRandomizer::nextPiece() {
    if (m_bag.empty()) {
        refillBag();
    }
    char shape = m_bag.back();
    m_bag.pop_back();

    m_piecesSpawned++;
    uint8_t col = pickColor(m_piecesSpawned);

    return std::make_unique<Tetromino>(shape, col);
}

std::vector<std::unique_ptr<Tetromino>> BagRandomizer::peekNext(int count) {
    std::vector<std::unique_ptr<Tetromino>> previews;
    auto tempBag = m_bag;
    int simCount = m_piecesSpawned;

    while (static_cast<int>(previews.size()) < count) {
        if (tempBag.empty()) {
            tempBag = {'I', 'O', 'T', 'S', 'Z', 'J', 'L'};
            static std::random_device rd;
            static std::mt19937 g(rd());
            std::shuffle(tempBag.begin(), tempBag.end(), g);
        }
        char shape = tempBag.back();
        tempBag.pop_back();

        simCount++;
        uint8_t col = static_cast<uint8_t>((std::hash<char>{}(shape) + simCount) % m_numColors + 1);
        if (m_enableElements) {
            if (simCount % 7 == 0) col = MATERIAL_WATER;
            else if (simCount % 13 == 0) col = MATERIAL_ACID;
        }

        previews.push_back(std::make_unique<Tetromino>(shape, col));
    }
    return previews;
}
