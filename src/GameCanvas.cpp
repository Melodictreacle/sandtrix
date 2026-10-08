#include "GameCanvas.h"
#include <QPainter>
#include <QFont>
#include <cmath>
#include <numeric>

GameCanvas::GameCanvas(AudioManager* audio, const ColorPalette& initialPalette, QWidget* parent)
    : QWidget(parent),
      m_audio(audio),
      m_palette(initialPalette),
      m_engine(static_cast<int>(initialPalette.colors.size() - 1)),
      m_sandImage(BOARD_WIDTH, BOARD_HEIGHT, QImage::Format_RGB32),
      m_keyLeftHeld(false),
      m_keyRightHeld(false),
      m_dasTimer(0),
      m_arrTimer(0),
      m_activeDir(0),
      m_pulseTime(0.0),
      m_gameOverSoundPlayed(false),
      m_victorySoundPlayed(false),
      m_confusionTimer(0),
      m_dangerRoastTimer(0)
{
    setFixedSize(CANVAS_WIDTH, CANVAS_HEIGHT);
    setFocusPolicy(Qt::StrongFocus);

    connect(&m_timer, &QTimer::timeout, this, &GameCanvas::onGameLoop);
    m_timer.start(FRAME_TIME_MS);
}

void GameCanvas::setPalette(const ColorPalette& palette) {
    m_palette = palette;
    int numColors = static_cast<int>(palette.colors.size() - 1);
    m_engine.setPaletteColorsCount(numColors);
    m_particles.clear();
    update();
    emit stateChanged();
}

void GameCanvas::applySettings(const GameSettings& settings) {
    m_palette = getGamePalette();
    int numColors = static_cast<int>(m_palette.colors.size() - 1);
    m_engine.applySettings(settings, numColors);
    m_particles.clear();
    m_gameOverSoundPlayed = false;
    m_victorySoundPlayed = false;
    m_confusionTimer = 0;
    m_dangerRoastTimer = 0;
    update();
    emit stateChanged();
}

void GameCanvas::restartGame() {
    m_engine.reset();
    m_particles.clear();
    m_gameOverSoundPlayed = false;
    m_victorySoundPlayed = false;
    m_confusionTimer = 0;
    m_dangerRoastTimer = 0;
    update();
    emit stateChanged();
}

void GameCanvas::onGameLoop() {
    m_pulseTime += 0.05;
    int dt = FRAME_TIME_MS;

    // DAS / ARR horizontal repeat
    if (m_keyLeftHeld || m_keyRightHeld) {
        m_dasTimer += dt;
        if (m_dasTimer >= DAS_DELAY_MS) {
            m_arrTimer += dt;
            if (m_arrTimer >= ARR_REPEAT_MS) {
                m_arrTimer = 0;
                int dir = (m_confusionTimer > 0) ? -m_activeDir : m_activeDir;
                if (m_engine.movePiece(dir * MINO_SIZE)) {
                    if (m_audio) m_audio->playMove();
                }
            }
        }
    }

    // Engine tick
    EngineTickEvent event = m_engine.tick(dt);

    if (m_confusionTimer > 0) {
        m_confusionTimer -= dt;
    }

    if (event.confusionTriggered) {
        m_confusionTimer = 6000;
        static const char* confMsgs[] = {
            "CONTROLS INVERTED!", "RIGHT IS LEFT, LEFT IS RIGHT!", "BRAIN SCRAMBLE!", "CONFUSION 100!"
        };
        static int cIdx = 0;
        m_particles.addFloatingText(CANVAS_WIDTH / 2.0f - 85.0f, 160.0f, confMsgs[cIdx++ % 4], QColor(255, 80, 80), 16);
        m_particles.triggerShake(5.0f);
    }

    if (event.shapeShifted) {
        if (m_audio) m_audio->playMorph();
        Tetromino* active = m_engine.getActivePiece();
        int px = active ? (active->getX() * CELL_DISPLAY_SIZE + 20) : (CANVAS_WIDTH / 2);
        int py = active ? (active->getY() * CELL_DISPLAY_SIZE + 10) : 180;
        static const char* morphRoasts[] = {
            "SHAPE SHIFT!", "SURPRISE!", "ADAPT OR DIE!", "NO TETRIS FOR YOU!", "NICE PLAN... OOPS!", "MUTATION!"
        };
        static int mIdx = 0;
        m_particles.addFloatingText(px - 50.0f, py - 20.0f, morphRoasts[mIdx++ % 6], QColor(255, 105, 180), 16);
        m_particles.triggerShake(4.0f);
    }

    if (event.bombDetonated) {
        if (m_audio) m_audio->playBoom();
        m_particles.triggerShake(12.0f);
        m_particles.addClearedSandSparks(event.bombCraterCoords, QColor(255, 100, 30));
        static const char* bombRoasts[] = {
            "BOOM! -CRATER", "THERE GOES YOUR SETUP!", "DEMOLITION TIME!", "HOPE THAT WASN'T IMPORTANT!"
        };
        static int bIdx = 0;
        m_particles.addFloatingText(event.bombX * CELL_DISPLAY_SIZE - 55.0f, event.bombY * CELL_DISPLAY_SIZE - 15.0f, bombRoasts[bIdx++ % 4], QColor(255, 75, 40), 17);
    }

    if (event.earthquake) {
        if (m_audio) m_audio->playDrop();
        m_particles.triggerShake(9.0f);
        static const char* quakes[] = {
            "EARTHQUAKE!", "HOLD ONTO YOUR SEATS!", "SEISMIC ACTIVITY!", "TREMOR DETECTED!"
        };
        static int qIdx = 0;
        m_particles.addFloatingText(CANVAS_WIDTH / 2.0f - 75.0f, 150.0f, quakes[qIdx++ % 4], QColor(245, 185, 35), 18);
    }

    if (event.windGust) {
        static const char* windRoasts[] = {
            "GUST!", "OOPS, SLIPPED!", "WIND BLOWS!", "WILD GUST APPEARS!"
        };
        static int wIdx = 0;
        QString wStr = QString("%1 %2 %3").arg((event.windDir > 0) ? ">>>" : "<<<").arg(windRoasts[wIdx++ % 4]).arg((event.windDir > 0) ? ">>>" : "<<<");
        m_particles.addFloatingText(CANVAS_WIDTH / 2.0f - 60.0f, 140.0f, wStr, QColor(0, 210, 215), 15);
    }

    if (event.landedWater) {
        if (m_audio) m_audio->playWaterSplash();
    } else if (event.landedAcid) {
        if (m_audio) m_audio->playAcidSizzle();
    } else if (event.landed && !event.bombDetonated) {
        if (m_audio) m_audio->playLock();
    }

    if (!event.acidCorrodedCoords.empty()) {
        m_particles.addAcidBubbles(event.acidCorrodedCoords);
        if (m_audio) m_audio->playAcidSizzle();
    }

    if (event.cleared) {
        if (event.isClassicTetris) {
            static const char* classicNames[] = { "", "SINGLE!", "DOUBLE!", "TRIPLE!", "TETRIS!" };
            int l = std::min(4, std::max(1, event.classicLines));
            QString title = QString("%1 +%2").arg(classicNames[l]).arg(event.points);
            QColor flashCol = (l == 4) ? QColor(245, 185, 35) : m_palette.colors[l % m_palette.colors.size()];

            int midY = 200;
            if (!event.coords.empty()) {
                int sumY = 0;
                for (const auto& pt : event.coords) sumY += pt.first;
                midY = (sumY / static_cast<int>(event.coords.size())) * CELL_DISPLAY_SIZE;
            }

            m_particles.addFloatingText(
                CANVAS_WIDTH / 2.0f - 55.0f, static_cast<float>(midY),
                title, flashCol, (l == 4) ? 18 : 15
            );

            if (m_engine.isMystery()) {
                static const char* singleRoasts[] = {
                    "ONLY ONE?", "BARE MINIMUM!", "IS THAT ALL?", "WEAK PLAY!", "1 LINE... HOW CUTE.", "NOT IMPRESSED."
                };
                static const char* doubleRoasts[] = {
                    "MEDIOCRE!", "STILL NOT A TETRIS.", "MEH, COULD BE BETTER.", "BABY STEPS!", "TWO LINES? YAWN."
                };
                static const char* tripleRoasts[] = {
                    "CHOKED ON THE 4TH!", "SO CLOSE YET SO FAR!", "ALMOST A TETRIS... BUT NOT.", "GREEDY MUCH?"
                };
                static const char* tetrisPraises[] = {
                    "TOTAL ACCIDENT, RIGHT?", "CALCULATED LUCK!", "WAIT, THAT WORKED?!", "BIG BRAIN TIME!", "DON'T GET COCKY NOW!"
                };

                static int roastIdx = 0;
                const char* roast = nullptr;
                if (l == 1) roast = singleRoasts[(roastIdx++) % 6];
                else if (l == 2) roast = doubleRoasts[(roastIdx++) % 5];
                else if (l == 3) roast = tripleRoasts[(roastIdx++) % 4];
                else if (l == 4) roast = tetrisPraises[(roastIdx++) % 5];

                if (roast) {
                    m_particles.addFloatingText(
                        CANVAS_WIDTH / 2.0f - 75.0f, static_cast<float>(midY + 24),
                        roast, (l == 4) ? QColor(255, 215, 0) : QColor(255, 120, 120), 13
                    );
                }
            }

            m_particles.addClearedSandSparks(event.coords, flashCol);
            m_particles.triggerShake(3.0f + l * 2.0f);
            if (m_audio) m_audio->playClear(l);
        } else {
            QColor col;
            if (event.isTidalWave) {
                col = getWaterColor();
                m_particles.addWaterSplash(CANVAS_WIDTH / 2.0f, 300.0f, 60);
                if (m_audio) m_audio->playWaterSplash();
            } else {
                col = m_palette.colors[event.color % m_palette.colors.size()];
            }

            m_particles.addClearedSandSparks(event.coords, col);

            // Calculate vertical center of cleared sand for floating text
            int midY = 200;
            if (!event.coords.empty()) {
                int sumY = 0;
                for (const auto& pt : event.coords) sumY += pt.first;
                midY = (sumY / static_cast<int>(event.coords.size())) * CELL_DISPLAY_SIZE;
            }

            if (event.isTidalWave) {
                m_particles.addFloatingText(
                    CANVAS_WIDTH / 2.0f - 65.0f, static_cast<float>(midY),
                    QString("TIDAL WAVE! +%1").arg(event.points),
                    col, 16
                );
            } else if (event.combo > 1) {
                m_particles.addFloatingText(
                    CANVAS_WIDTH / 2.0f - 50.0f, static_cast<float>(midY),
                    QString("COMBO x%1! +%2").arg(event.combo).arg(event.points),
                    col, 15
                );
                if (m_engine.isMystery()) {
                    static const char* hypeQuotes[] = { "ACCIDENTAL GENIUS!", "CALCULATED!", "WAIT, THAT WORKED?!", "BIG BRAIN!" };
                    static int hIdx = 0;
                    m_particles.addFloatingText(
                        CANVAS_WIDTH / 2.0f - 65.0f, static_cast<float>(midY + 22),
                        hypeQuotes[hIdx++ % 4], QColor(255, 215, 0), 13
                    );
                }
            } else {
                m_particles.addFloatingText(
                    CANVAS_WIDTH / 2.0f - 35.0f, static_cast<float>(midY),
                    QString("+%1").arg(event.points),
                    col, 14
                );
            }

            m_particles.triggerShake(3.0f + std::min(10.0f, event.combo * 2.0f));
            if (m_audio) m_audio->playClear(event.combo);
        }
    }

    // Check near-ceiling danger warning roast in Troll / Mystery mode
    if (m_engine.isMystery() && !m_engine.isGameOver() && !m_engine.isPaused()) {
        m_dangerRoastTimer += dt;
        if (m_dangerRoastTimer >= 5500) {
            m_dangerRoastTimer = 0;
            const auto& grid = m_engine.getGrid();
            int ceilingCheckLimit = m_engine.isClassicTetris() ? (4 + 4 * MINO_SIZE) : 32;
            bool nearCeiling = false;
            for (int r = 0; r < ceilingCheckLimit; ++r) {
                for (int c = 0; c < BOARD_WIDTH; ++c) {
                    if (grid[r * BOARD_WIDTH + c] > 0) {
                        nearCeiling = true;
                        break;
                    }
                }
                if (nearCeiling) break;
            }

            if (nearCeiling) {
                static const char* dangerRoasts[] = {
                    "SWEATING YET?",
                    "LIVING ON THE EDGE!",
                    "DON'T CHOKE NOW!",
                    "CHOKING HAZARD DETECTED!",
                    "PANIC STACKING INTENSIFIES!"
                };
                static int drIdx = 0;
                m_particles.addFloatingText(
                    CANVAS_WIDTH / 2.0f - 75.0f, 95.0f,
                    dangerRoasts[drIdx++ % 5], QColor(255, 140, 50), 15
                );
                m_particles.triggerShake(3.0f);
            }
        }
    }

    if (event.missionCompleted || (m_engine.isMissionCompleted() && !m_victorySoundPlayed)) {
        if (m_audio) m_audio->playClear(4);
        m_victorySoundPlayed = true;
        m_particles.triggerShake(6.0f);
        m_particles.addFloatingText(CANVAS_WIDTH / 2.0f - 85.0f, 150.0f, "MISSION COMPLETE!", QColor(255, 215, 0), 20);
    } else if (!m_engine.isMissionCompleted()) {
        m_victorySoundPlayed = false;
    }

    if (m_engine.isGameOver() && !m_gameOverSoundPlayed) {
        if (m_audio) m_audio->playGameOver();
        m_gameOverSoundPlayed = true;
    } else if (!m_engine.isGameOver()) {
        m_gameOverSoundPlayed = false;
    }

    update();
    emit stateChanged();
}

void GameCanvas::keyPressEvent(QKeyEvent* event) {
    int key = event->key();

    if (key == Qt::Key_R) {
        restartGame();
        return;
    }

    if (key == Qt::Key_P || key == Qt::Key_Escape) {
        if (!m_engine.isMissionCompleted() && !m_engine.isGameOver()) {
            m_engine.togglePaused();
            update();
            emit stateChanged();
        }
        return;
    }

    if (key == Qt::Key_M) {
        if (m_audio) m_audio->toggleMute();
        return;
    }

    if (m_engine.isGameOver() || m_engine.isMissionCompleted() || m_engine.isPaused()) {
        return;
    }

    m_engine.recordKeyPress();

    bool inverted = (m_confusionTimer > 0);

    if (key == Qt::Key_Left || key == Qt::Key_A) {
        if (!m_keyLeftHeld) {
            m_keyLeftHeld = true;
            m_activeDir = -1;
            m_dasTimer = 0;
            m_arrTimer = 0;
            int dir = inverted ? MINO_SIZE : -MINO_SIZE;
            if (m_engine.movePiece(dir)) {
                if (m_audio) m_audio->playMove();
            }
        }
    } else if (key == Qt::Key_Right || key == Qt::Key_D) {
        if (!m_keyRightHeld) {
            m_keyRightHeld = true;
            m_activeDir = 1;
            m_dasTimer = 0;
            m_arrTimer = 0;
            int dir = inverted ? -MINO_SIZE : MINO_SIZE;
            if (m_engine.movePiece(dir)) {
                if (m_audio) m_audio->playMove();
            }
        }
    } else if (key == Qt::Key_Up || key == Qt::Key_W || key == Qt::Key_X) {
        int rotDir = inverted ? -1 : 1;
        if (m_engine.rotatePiece(rotDir)) {
            if (m_audio) m_audio->playRotate();
        }
    } else if (key == Qt::Key_Z || key == Qt::Key_Control) {
        int rotDir = inverted ? 1 : -1;
        if (m_engine.rotatePiece(rotDir)) {
            if (m_audio) m_audio->playRotate();
        }
    } else if (key == Qt::Key_Down || key == Qt::Key_S) {
        m_engine.setSoftDropping(true);
    } else if (key == Qt::Key_Space) {
        int lx = 0, ly = 0;
        std::vector<std::pair<int, int>> impactCoords;
        bool wasWater = (m_engine.getActivePiece() && m_engine.getActivePiece()->isWater());
        bool wasAcid = (m_engine.getActivePiece() && m_engine.getActivePiece()->isAcid());
        bool wasBomb = (m_engine.getActivePiece() && m_engine.getActivePiece()->isBomb());

        if (m_engine.hardDrop(lx, ly, impactCoords)) {
            if (wasWater) {
                if (m_audio) m_audio->playWaterSplash();
                m_particles.addWaterSplash(lx * CELL_DISPLAY_SIZE + 20, ly * CELL_DISPLAY_SIZE, 35);
            } else if (wasAcid) {
                if (m_audio) m_audio->playAcidSizzle();
            } else if (wasBomb) {
                if (m_audio) m_audio->playBoom();
                m_particles.triggerShake(12.0f);
                m_particles.addFloatingText(lx * CELL_DISPLAY_SIZE - 20, ly * CELL_DISPLAY_SIZE - 25, "KABOOM!", QColor(255, 75, 40), 18);
            } else {
                if (m_audio) m_audio->playDrop();
                if (!impactCoords.empty()) {
                    QColor col = m_palette.colors[1 % m_palette.colors.size()];
                    m_particles.addLandingDust(lx * CELL_DISPLAY_SIZE + 20, ly * CELL_DISPLAY_SIZE, col);
                }
            }
            m_particles.triggerShake(4.0f);

            if (m_engine.isMystery() && !wasBomb) {
                static const char* dropQuotes[] = { "SLAMMED!", "FULL SEND!", "SEND IT!", "ZERO REGRETS!", "NO FEAR!" };
                static int qIdx = 0;
                m_particles.addFloatingText(lx * CELL_DISPLAY_SIZE - 20, ly * CELL_DISPLAY_SIZE - 25, dropQuotes[qIdx++ % 5], QColor(255, 180, 50), 14);
            }
        }
    } else if (key == Qt::Key_C || key == Qt::Key_Shift) {
        if (m_engine.holdCurrentPiece()) {
            if (m_audio) m_audio->playMove();
        }
    }
}

void GameCanvas::keyReleaseEvent(QKeyEvent* event) {
    int key = event->key();

    if (key == Qt::Key_Left || key == Qt::Key_A) {
        m_keyLeftHeld = false;
        if (m_keyRightHeld) {
            m_activeDir = 1;
            m_dasTimer = 0;
        }
    } else if (key == Qt::Key_Right || key == Qt::Key_D) {
        m_keyRightHeld = false;
        if (m_keyLeftHeld) {
            m_activeDir = -1;
            m_dasTimer = 0;
        }
    } else if (key == Qt::Key_Down || key == Qt::Key_S) {
        m_engine.setSoftDropping(false);
    }
}

void GameCanvas::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Apply screen shake
    auto [shakeX, shakeY] = m_particles.update();
    painter.translate(shakeX, shakeY);

    // 1. Copy grid to QImage RGB32 buffer
    const auto& grid = m_engine.getGrid();
    uint32_t* bits = reinterpret_cast<uint32_t*>(m_sandImage.bits());
    size_t numPalColors = m_palette.colors.size();

    std::vector<uint32_t> paletteRgb(numPalColors);
    for (size_t i = 0; i < numPalColors; ++i) {
        paletteRgb[i] = m_palette.colors[i].rgb();
    }
    uint32_t waterRgb = getWaterColor().rgb();
    uint32_t acidRgb = getAcidColor().rgb();
    uint32_t bombRgb = getBombColor().rgb();

    for (int i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; ++i) {
        uint8_t c = grid[i];
        if (c == MATERIAL_WATER) {
            bits[i] = waterRgb;
        } else if (c == MATERIAL_ACID) {
            bits[i] = acidRgb;
        } else if (c == MATERIAL_BOMB) {
            bits[i] = bombRgb;
        } else {
            bits[i] = paletteRgb[c % numPalColors];
        }
    }

    // Draw scaled sand image
    painter.drawImage(QRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT), m_sandImage);

    // In Classic Tetris mode: draw subtle 10x17 mino grid lines!
    if (m_engine.isClassicTetris()) {
        painter.setPen(QPen(QColor(255, 255, 255, 14), 1, Qt::DotLine));
        // Vertical lines (every 8 grains = 40px)
        for (int c = 1; c < 10; ++c) {
            int px = c * MINO_SIZE * CELL_DISPLAY_SIZE;
            painter.drawLine(px, 4 * CELL_DISPLAY_SIZE, px, CANVAS_HEIGHT);
        }
        // Horizontal lines (every 8 grains = 40px, starting at y = 4 grains = 20px)
        for (int r = 0; r <= 17; ++r) {
            int py = (4 + r * MINO_SIZE) * CELL_DISPLAY_SIZE;
            painter.drawLine(0, py, CANVAS_WIDTH, py);
        }
    }

    // 2. Subtle border
    painter.setPen(QPen(QColor(42, 46, 63, 160), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(0, 0, CANVAS_WIDTH - 1, CANVAS_HEIGHT - 1);

    // Inverted controls warning banner
    if (m_confusionTimer > 0) {
        painter.setFont(QFont("Segoe UI", 9, QFont::Bold));
        painter.setPen(QColor(255, 90, 90));
        int sec = (m_confusionTimer + 999) / 1000;
        painter.drawText(QRect(0, 6, CANVAS_WIDTH, 22), Qt::AlignCenter, QString("CONTROLS INVERTED (%1s)").arg(sec));
    }

    // 3. Danger Ceiling Line
    int dangerY = (m_engine.isClassicTetris() ? (4 + MINO_SIZE) : DANGER_ROW) * CELL_DISPLAY_SIZE;
    int pulseAlpha = static_cast<int>(120 + 80 * std::sin(m_pulseTime * 3.0));
    painter.setPen(QPen(QColor(255, 60, 60, pulseAlpha), 1, Qt::DashLine));
    painter.drawLine(0, dangerY, CANVAS_WIDTH, dangerY);

    // 4. Ghost Piece
    Tetromino* active = m_engine.getActivePiece();
    if (active && !m_engine.isGameOver()) {
        int ghostY = m_engine.getActiveGhostY();
        if (ghostY != active->getY()) {
            QColor ghostCol;
            if (active->isWater()) ghostCol = getWaterColor();
            else if (active->isAcid()) ghostCol = getAcidColor();
            else if (active->isBomb()) ghostCol = getBombColor();
            else ghostCol = m_palette.colors[active->getColorIdx() % numPalColors];

            ghostCol.setAlpha(65);
            painter.setPen(Qt::NoPen);
            painter.setBrush(ghostCol);

            int mH = active->getMaskHeight();
            int mW = active->getMaskWidth();
            const auto& mask = active->getGrainMask();

            for (int gy = 0; gy < mH; ++gy) {
                for (int gx = 0; gx < mW; ++gx) {
                    if (mask[gy * mW + gx]) {
                        int px = (active->getX() + gx) * CELL_DISPLAY_SIZE;
                        int py = (ghostY + gy) * CELL_DISPLAY_SIZE;
                        painter.drawRect(px, py, CELL_DISPLAY_SIZE, CELL_DISPLAY_SIZE);
                    }
                }
            }
        }
    }

    // 5. Active Falling Tetromino
    if (active && !m_engine.isGameOver()) {
        QColor activeCol;
        if (active->isWater()) activeCol = getWaterColor();
        else if (active->isAcid()) activeCol = getAcidColor();
        else if (active->isBomb()) {
            int flash = static_cast<int>(m_pulseTime * 8) % 2;
            activeCol = (flash == 0) ? getBombColor() : QColor(255, 180, 30);
        } else activeCol = m_palette.colors[active->getColorIdx() % numPalColors];

        painter.setPen(Qt::NoPen);
        painter.setBrush(activeCol);

        int mH = active->getMaskHeight();
        int mW = active->getMaskWidth();
        const auto& mask = active->getGrainMask();

        for (int gy = 0; gy < mH; ++gy) {
            for (int gx = 0; gx < mW; ++gx) {
                if (mask[gy * mW + gx]) {
                    int px = (active->getX() + gx) * CELL_DISPLAY_SIZE;
                    int py = (active->getY() + gy) * CELL_DISPLAY_SIZE;
                    painter.drawRect(px, py, CELL_DISPLAY_SIZE, CELL_DISPLAY_SIZE);
                }
            }
        }
    }

    // 6. Particles & Floating Texts
    m_particles.draw(painter);

    // 7. Overlays
    if (m_engine.isMissionCompleted()) {
        drawMissionCompleteOverlay(painter);
    } else if (m_engine.isPaused()) {
        drawPauseOverlay(painter);
    } else if (m_engine.isGameOver()) {
        drawGameOverOverlay(painter);
    }
}

void GameCanvas::drawPauseOverlay(QPainter& painter) {
    painter.fillRect(rect(), QColor(10, 12, 18, 190));

    painter.setFont(QFont("Segoe UI", 26, QFont::Bold));
    painter.setPen(QColor("#7cb7ea"));
    painter.drawText(rect(), Qt::AlignCenter, "PAUSED");

    painter.setFont(QFont("Segoe UI", 12));
    painter.setPen(QColor("#cbd5e0"));
    QRect subRect = rect().adjusted(0, 60, 0, 0);
    painter.drawText(subRect, Qt::AlignCenter, "Press P or Esc to Resume");
}

void GameCanvas::drawStatsTable(QPainter& painter, int startY, bool isVictory) {
    const RunStats& stats = m_engine.getRunStats();

    QRect tableRect(20, startY, 360, 366);
    painter.setPen(QPen(QColor("#283248"), 1));
    painter.setBrush(QColor(18, 23, 34, 240));
    painter.drawRoundedRect(tableRect, 10, 10);

    struct StatEntry {
        QString label;
        QString value;
        QColor valCol;
    };

    QString goalStr;
    if (stats.lineGoal > 0) {
        if (isVictory) {
            goalStr = QString("%1 Lines (COMPLETED)").arg(stats.lineGoal);
        } else {
            int pct = (stats.lineGoal > 0) ? static_cast<int>(stats.linesCleared * 100.0 / stats.lineGoal) : 0;
            goalStr = QString("%1 / %2 (%3%)").arg(stats.linesCleared).arg(stats.lineGoal).arg(pct);
        }
    } else {
        goalStr = QString("%1 Lines (Endless)").arg(stats.linesCleared);
    }

    std::vector<StatEntry> entries = {
        { "TIME TAKEN", stats.getFormattedTime(), QColor("#63b3ed") },
        { "KEYS PRESSED", QString::number(stats.keysPressed), QColor("#ecc94b") },
        { "GOAL", goalStr, isVictory ? QColor("#48bb78") : QColor("#cbd5e0") },
        { "PIECES PLACED", QString::number(stats.piecesPlaced), QColor("#e2e8f0") },
        { "SPEED (PPS)", QString("%1 pcs/s").arg(stats.getPPS(), 0, 'f', 2), QColor("#38b2ac") },
        { "FINESSE (KPP)", QString("%1 keys/pc").arg(stats.getKPP(), 0, 'f', 2), QColor("#ed8936") },
        { "ACCURACY", QString("%1% (%2 l/pc)").arg(stats.getAccuracyPct(), 0, 'f', 1).arg(stats.getLinesPerPiece(), 0, 'f', 2), QColor("#b794f4") },
        { "MAX COMBO", QString("x%1").arg(stats.maxCombo), QColor("#f687b3") },
        { "FINAL SCORE", QLocale().toString(stats.score), QColor("#f6ad55") },
        { "HIGH SCORE", QLocale().toString(m_engine.getHighScore()), QColor("#f6e05e") }
    };

    int colW = 168;
    int cellH = 64;
    int leftPad = 28;

    for (size_t i = 0; i < entries.size(); ++i) {
        int r = static_cast<int>(i / 2);
        int c = static_cast<int>(i % 2);
        int x = leftPad + c * (colW + 10);
        int y = startY + 12 + r * cellH;

        QRect cellBox(x, y, colW, cellH - 8);
        painter.setPen(QPen(QColor("#242d40"), 1));
        painter.setBrush(QColor(12, 16, 24, 180));
        painter.drawRoundedRect(cellBox, 6, 6);

        painter.setFont(QFont("Segoe UI", 8, QFont::Bold));
        painter.setPen(QColor("#94a1b2"));
        painter.drawText(QRect(x + 8, y + 6, colW - 16, 16), Qt::AlignLeft | Qt::AlignVCenter, entries[i].label);

        painter.setFont(QFont("Segoe UI", 11, QFont::Bold));
        painter.setPen(entries[i].valCol);
        painter.drawText(QRect(x + 8, y + 24, colW - 16, 26), Qt::AlignLeft | Qt::AlignVCenter, entries[i].value);
    }
}

void GameCanvas::drawMissionCompleteOverlay(QPainter& painter) {
    painter.fillRect(rect(), QColor(8, 14, 20, 230));

    // Victory Banner
    QRect bannerRect(20, 38, 360, 78);
    painter.setPen(QPen(QColor("#2f855a"), 2));
    painter.setBrush(QColor("#132a1f"));
    painter.drawRoundedRect(bannerRect, 10, 10);

    painter.setFont(QFont("Segoe UI", 21, QFont::Bold));
    painter.setPen(QColor("#48bb78"));
    painter.drawText(QRect(20, 46, 360, 32), Qt::AlignCenter, "MISSION COMPLETE!");

    const RunStats& stats = m_engine.getRunStats();
    painter.setFont(QFont("Segoe UI", 11, QFont::Bold));
    painter.setPen(QColor("#9ae6b4"));
    painter.drawText(QRect(20, 82, 360, 22), Qt::AlignCenter,
        QString("GOAL REACHED: %1 / %1 LINES").arg(stats.lineGoal));

    // Stats Table
    drawStatsTable(painter, 126, true);

    // Call to Action
    int restartAlpha = static_cast<int>(170 + 75 * std::sin(m_pulseTime * 4.0));
    painter.setFont(QFont("Segoe UI", 12, QFont::Bold));
    painter.setPen(QColor(124, 183, 234, restartAlpha));
    painter.drawText(QRect(0, 508, CANVAS_WIDTH, 26), Qt::AlignCenter, "Press [R] to Play Again");

    painter.setFont(QFont("Segoe UI", 10));
    painter.setPen(QColor("#a0aec0"));
    painter.drawText(QRect(0, 536, CANVAS_WIDTH, 20), Qt::AlignCenter, "Press [Esc] for Main Menu");
}

void GameCanvas::drawGameOverOverlay(QPainter& painter) {
    painter.fillRect(rect(), QColor(15, 8, 12, 230));

    // Header Banner
    QRect bannerRect(20, 38, 360, 78);
    painter.setPen(QPen(QColor("#742a2a"), 2));
    painter.setBrush(QColor("#2d1519"));
    painter.drawRoundedRect(bannerRect, 10, 10);

    painter.setFont(QFont("Segoe UI", 21, QFont::Bold));
    painter.setPen(QColor("#e27d9a"));
    painter.drawText(QRect(20, 46, 360, 32), Qt::AlignCenter, "GAME OVER");

    const RunStats& stats = m_engine.getRunStats();
    if (m_engine.isMystery()) {
        static const char* gameOverRoasts[] = {
            "SKILL ISSUE DETECTED",
            "REST IN PIECES",
            "F IN THE CHAT",
            "PERHAPS TETRIS ISN'T FOR YOU",
            "NICE TRY, BETTER LUCK NEXT TIME"
        };
        int rIdx = (stats.score / 250) % 5;
        painter.setFont(QFont("Segoe UI", 11, QFont::Bold));
        painter.setPen(QColor("#fc8181"));
        painter.drawText(QRect(20, 82, 360, 22), Qt::AlignCenter, gameOverRoasts[rIdx]);
    } else if (stats.lineGoal > 0) {
        int pct = (stats.lineGoal > 0) ? static_cast<int>(stats.linesCleared * 100.0 / stats.lineGoal) : 0;
        painter.setFont(QFont("Segoe UI", 11, QFont::Bold));
        painter.setPen(QColor("#cbd5e0"));
        painter.drawText(QRect(20, 82, 360, 22), Qt::AlignCenter,
            QString("MISSION PROGRESS: %1 / %2 LINES (%3%)").arg(stats.linesCleared).arg(stats.lineGoal).arg(pct));
    } else {
        painter.setFont(QFont("Segoe UI", 11, QFont::Bold));
        painter.setPen(QColor("#cbd5e0"));
        painter.drawText(QRect(20, 82, 360, 22), Qt::AlignCenter,
            QString("LINES CLEARED: %1").arg(stats.linesCleared));
    }

    // Stats Table
    drawStatsTable(painter, 126, false);

    // Call to Action
    int restartAlpha = static_cast<int>(170 + 75 * std::sin(m_pulseTime * 4.0));
    painter.setFont(QFont("Segoe UI", 12, QFont::Bold));
    painter.setPen(QColor(124, 183, 234, restartAlpha));
    painter.drawText(QRect(0, 508, CANVAS_WIDTH, 26), Qt::AlignCenter, "Press [R] to Retry");

    painter.setFont(QFont("Segoe UI", 10));
    painter.setPen(QColor("#a0aec0"));
    painter.drawText(QRect(0, 536, CANVAS_WIDTH, 20), Qt::AlignCenter, "Press [Esc] for Main Menu");
}
