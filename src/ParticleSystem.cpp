#include "ParticleSystem.h"
#include "Config.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <QFont>

namespace {
std::mt19937& getRng() {
    static std::random_device rd;
    static std::mt19937 g(rd());
    return g;
}

float randomFloat(float minVal, float maxVal) {
    std::uniform_real_distribution<float> dist(minVal, maxVal);
    return dist(getRng());
}
} // namespace

bool SparkParticle::update() {
    x += vx;
    y += vy;
    vy += gravity;
    vx *= friction;
    life--;
    alpha = std::max(0.0f, 255.0f * (static_cast<float>(life) / maxLife));
    return life > 0;
}

void SparkParticle::draw(QPainter& painter) const {
    QColor c = color;
    c.setAlpha(static_cast<int>(alpha));
    painter.setPen(Qt::NoPen);
    painter.setBrush(c);
    painter.drawRect(static_cast<int>(x), static_cast<int>(y), static_cast<int>(size), static_cast<int>(size));
}

bool FloatingText::update() {
    y += vy;
    vy *= 0.95f;
    life--;
    return life > 0;
}

void FloatingText::draw(QPainter& painter) const {
    int a = std::max(0, std::min(255, static_cast<int>(255 * (static_cast<float>(life) / maxLife))));
    QColor col = color;
    col.setAlpha(a);

    QFont font("Segoe UI", fontSize, QFont::Bold);
    painter.setFont(font);

    // Subtle drop shadow
    QColor shadowCol(0, 0, 0, static_cast<int>(a * 0.7f));
    painter.setPen(shadowCol);
    painter.drawText(static_cast<int>(x + 2), static_cast<int>(y + 2), text);

    painter.setPen(col);
    painter.drawText(static_cast<int>(x), static_cast<int>(y), text);
}

ParticleManager::ParticleManager()
    : m_shakeIntensity(0.0f)
{
}

void ParticleManager::addClearedSandSparks(const std::vector<std::pair<int, int>>& coords, const QColor& color) {
    if (coords.empty()) return;

    size_t step = std::max<size_t>(1, coords.size() / 120);
    for (size_t i = 0; i < coords.size(); i += step) {
        float px = coords[i].second * CELL_DISPLAY_SIZE;
        float py = coords[i].first * CELL_DISPLAY_SIZE;

        SparkParticle p;
        p.x = px + randomFloat(-2.0f, 2.0f);
        p.y = py + randomFloat(-2.0f, 2.0f);
        float speed = randomFloat(1.5f, 4.5f);
        p.vx = speed * randomFloat(-1.0f, 1.0f);
        p.vy = -std::abs(speed) * randomFloat(0.6f, 1.4f);
        p.gravity = 0.15f;
        p.friction = 0.96f;
        p.color = color;
        p.maxLife = static_cast<int>(randomFloat(25, 45));
        p.life = p.maxLife;
        p.alpha = 255.0f;
        p.size = randomFloat(2.0f, 3.5f);
        p.isBright = false;
        m_particles.push_back(p);

        if (randomFloat(0.0f, 1.0f) < 0.35f) {
            SparkParticle whiteHot = p;
            whiteHot.color = QColor(255, 255, 255);
            whiteHot.isBright = true;
            m_particles.push_back(whiteHot);
        }
    }
}

void ParticleManager::addLandingDust(float pixelX, float pixelY, const QColor& color, int count) {
    for (int i = 0; i < count; ++i) {
        SparkParticle p;
        p.x = pixelX + randomFloat(-12.0f, 12.0f);
        p.y = pixelY;
        p.vx = randomFloat(-1.5f, 1.5f);
        p.vy = randomFloat(-1.2f, -0.2f);
        p.gravity = 0.08f;
        p.friction = 0.94f;
        p.color = color;
        p.maxLife = 22;
        p.life = 22;
        p.alpha = 255.0f;
        p.size = 2.0f;
        p.isBright = false;
        m_particles.push_back(p);
    }
}

void ParticleManager::addFloatingText(float x, float y, const QString& text, const QColor& color, int fontSize) {
    FloatingText t;
    t.x = x;
    t.y = y;
    t.vy = -1.2f;
    t.text = text;
    t.color = color;
    t.fontSize = fontSize;
    t.maxLife = 40;
    t.life = 40;
    m_floatingTexts.push_back(t);
}

void ParticleManager::triggerShake(float intensity) {
    m_shakeIntensity = std::max(m_shakeIntensity, intensity);
}

std::pair<float, float> ParticleManager::update() {
    // Update and prune particles
    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(), [](SparkParticle& p) {
            return !p.update();
        }),
        m_particles.end()
    );

    // Update and prune floating texts
    m_floatingTexts.erase(
        std::remove_if(m_floatingTexts.begin(), m_floatingTexts.end(), [](FloatingText& t) {
            return !t.update();
        }),
        m_floatingTexts.end()
    );

    float offsetX = 0.0f;
    float offsetY = 0.0f;
    if (m_shakeIntensity > 0.1f) {
        offsetX = randomFloat(-m_shakeIntensity, m_shakeIntensity);
        offsetY = randomFloat(-m_shakeIntensity, m_shakeIntensity);
        m_shakeIntensity *= 0.85f;
    } else {
        m_shakeIntensity = 0.0f;
    }

    return {offsetX, offsetY};
}

void ParticleManager::draw(QPainter& painter) {
    for (const auto& p : m_particles) {
        p.draw(painter);
    }
    for (const auto& t : m_floatingTexts) {
        t.draw(painter);
    }
}

void ParticleManager::clear() {
    m_particles.clear();
    m_floatingTexts.clear();
    m_shakeIntensity = 0.0f;
}
