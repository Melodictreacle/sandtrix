#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include "Config.h"

struct GrainCoord {
    int y;
    int x;
    uint8_t color;
};

class Tetromino {
public:
    Tetromino(char shape, uint8_t colorIdx);

    int getX() const { return m_x; }
    int getY() const { return m_y; }
    void setX(int x) { m_x = x; }
    void setY(int y) { m_y = y; }
    char getShape() const { return m_shape; }
    uint8_t getColorIdx() const { return m_colorIdx; }
    int getRotation() const { return m_rotation; }

    int getMaskWidth() const { return m_maskWidth; }
    int getMaskHeight() const { return m_maskHeight; }
    const std::vector<bool>& getGrainMask() const { return m_grainMask; }

    bool collides(const std::vector<uint8_t>& grid, int offsetX = 0, int offsetY = 0, int testRotation = -1) const;
    bool tryMove(int dx, int dy, const std::vector<uint8_t>& grid);
    bool tryRotate(int direction, const std::vector<uint8_t>& grid);
    int getGhostY(const std::vector<uint8_t>& grid) const;
    std::vector<GrainCoord> getOccupiedGrains() const;

private:
    void updateGrainMask();

    char m_shape;
    uint8_t m_colorIdx;
    int m_rotation; // 0, 1, 2, 3
    int m_x;        // grain coordinates
    int m_y;

    int m_matrixSize;
    int m_maskWidth;
    int m_maskHeight;
    std::vector<bool> m_grainMask; // size: m_maskHeight * m_maskWidth
};

class BagRandomizer {
public:
    explicit BagRandomizer(int numColors = 4);
    void setNumColors(int numColors) { m_numColors = numColors; }
    std::unique_ptr<Tetromino> nextPiece();
    std::vector<std::unique_ptr<Tetromino>> peekNext(int count);

private:
    void refillBag();

    int m_numColors;
    std::vector<char> m_bag;
};
