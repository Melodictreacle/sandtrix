#pragma once

#include <QWidget>
#include <QTimer>
#include <QImage>
#include <QKeyEvent>
#include <memory>
#include "Config.h"
#include "SandEngine.h"
#include "ParticleSystem.h"
#include "AudioManager.h"

class GameCanvas : public QWidget {
    Q_OBJECT

public:
    explicit GameCanvas(AudioManager* audio, const ColorPalette& initialPalette, QWidget* parent = nullptr);

    SandEngine& getEngine() { return m_engine; }
    const SandEngine& getEngine() const { return m_engine; }
    void setPalette(const ColorPalette& palette);
    const ColorPalette& getPalette() const { return m_palette; }
    void applySettings(const GameSettings& settings);

    void restartGame();

signals:
    void stateChanged();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void onGameLoop();

private:
    void drawPauseOverlay(QPainter& painter);
    void drawGameOverOverlay(QPainter& painter);

    AudioManager* m_audio;
    ColorPalette m_palette;
    SandEngine m_engine;
    ParticleManager m_particles;

    QTimer m_timer;
    QImage m_sandImage;

    bool m_keyLeftHeld;
    bool m_keyRightHeld;
    int m_dasTimer;
    int m_arrTimer;
    int m_activeDir; // -1 left, +1 right

    double m_pulseTime;
    bool m_gameOverSoundPlayed;
    int m_confusionTimer; // Ms remaining for inverted controls in Mystery mode
    int m_dangerRoastTimer; // Ms timer between near-ceiling roasts
};
