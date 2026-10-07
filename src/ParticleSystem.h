#pragma once

#include <vector>
#include <utility>
#include <QString>
#include <QColor>
#include <QPainter>

struct SparkParticle {
    float x;
    float y;
    float vx;
    float vy;
    float gravity;
    float friction;
    float alpha;
    float size;
    QColor color;
    int life;
    int maxLife;
    bool isBright;

    bool update();
    void draw(QPainter& painter) const;
};

struct FloatingText {
    float x;
    float y;
    float vy;
    QString text;
    QColor color;
    int life;
    int maxLife;
    int fontSize;

    bool update();
    void draw(QPainter& painter) const;
};

class ParticleManager {
public:
    ParticleManager();

    void addClearedSandSparks(const std::vector<std::pair<int, int>>& coords, const QColor& color);
    void addWaterSplash(float pixelX, float pixelY, int count = 30);
    void addAcidBubbles(const std::vector<std::pair<int, int>>& coords);
    void addLandingDust(float pixelX, float pixelY, const QColor& color, int count = 15);
    void addFloatingText(float x, float y, const QString& text, const QColor& color, int fontSize = 14);
    void triggerShake(float intensity = 6.0f);

    std::pair<float, float> update();
    void draw(QPainter& painter);
    void clear();

private:
    std::vector<SparkParticle> m_particles;
    std::vector<FloatingText> m_floatingTexts;
    float m_shakeIntensity;
};
