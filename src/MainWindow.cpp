#include "MainWindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QTimer>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_canvas(nullptr)
{
    setWindowTitle("SANDTRIX - Sand Tetris for Qt");
    initStackedViews();

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::syncUI);
    timer->start(30);
}

void MainWindow::initStackedViews() {
    setStyleSheet(
        "QMainWindow {"
        "  background-color: #11141c;"
        "}"
        "QWidget {"
        "  color: #f0f4f8;"
        "  font-family: 'Segoe UI', system-ui, sans-serif;"
        "}"
        "QPushButton {"
        "  background-color: #242c3d;"
        "  border: 1px solid #37435f;"
        "  border-radius: 6px;"
        "  padding: 7px 12px;"
        "  color: #f7fafc;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #313c54;"
        "  border-color: #63b3ed;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1b212e;"
        "}"
    );

    m_stack = new QStackedWidget(this);

    // 0: Main Menu Screen
    m_mainMenu = new MainMenuWidget(this);
    m_mainMenu->updateSettingsBadge(m_settings);
    connect(m_mainMenu, &MainMenuWidget::startClicked, this, &MainWindow::onStartGame);
    connect(m_mainMenu, &MainMenuWidget::settingsClicked, this, &MainWindow::onOpenSettings);
    connect(m_mainMenu, &MainMenuWidget::howToPlayClicked, this, &MainWindow::onOpenHowToPlay);
    connect(m_mainMenu, &MainMenuWidget::exitClicked, this, &MainWindow::close);
    m_stack->addWidget(m_mainMenu);

    // 1: Settings Screen
    m_settingsView = new SettingsWidget(m_settings, this);
    connect(m_settingsView, &SettingsWidget::backClicked, this, &MainWindow::onReturnToMenu);
    connect(m_settingsView, &SettingsWidget::settingsChanged, this, &MainWindow::onSettingsChanged);
    m_stack->addWidget(m_settingsView);

    // 2: How To Play Screen
    m_howToPlayView = new HowToPlayWidget(this);
    connect(m_howToPlayView, &HowToPlayWidget::backClicked, this, &MainWindow::onReturnToMenu);
    m_stack->addWidget(m_howToPlayView);

    // 3: Game Screen
    m_gameScreen = createGameScreen();
    m_stack->addWidget(m_gameScreen);

    setCentralWidget(m_stack);
    setFixedSize(sizeHint());
}

QWidget* MainWindow::createGameScreen() {
    auto* gameContainer = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(gameContainer);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(16);

    const auto& defaultPalette = getGamePalette();

    // ----------------- LEFT PANEL -----------------
    auto* leftPanel = new QVBoxLayout();
    leftPanel->setSpacing(10);
    leftPanel->setAlignment(Qt::AlignTop);

    m_holdPreview = new PiecePreviewWidget("HOLD", defaultPalette, 84, this);
    leftPanel->addWidget(m_holdPreview);

    m_cardScore = new StatCard("Score", "0", "#327DEB", this);
    m_cardHighScore = new StatCard("High Score", "0", "#F5B923", this);
    m_cardLevel = new StatCard("Level", "1", "#28B964", this);
    m_cardBands = new StatCard("Lines Cleared", "0", "#EB4B4B", this);

    leftPanel->addWidget(m_cardScore);
    leftPanel->addWidget(m_cardHighScore);
    leftPanel->addWidget(m_cardLevel);
    leftPanel->addWidget(m_cardBands);

    m_comboBadge = new ComboBadge(this);
    leftPanel->addWidget(m_comboBadge);
    leftPanel->addStretch();

    // ----------------- CENTER CANVAS -----------------
    auto* centerPanel = new QVBoxLayout();
    centerPanel->setAlignment(Qt::AlignCenter);

    m_canvas = new GameCanvas(&m_audio, defaultPalette, this);
    centerPanel->addWidget(m_canvas);

    // ----------------- RIGHT PANEL -----------------
    auto* rightPanel = new QVBoxLayout();
    rightPanel->setSpacing(10);
    rightPanel->setAlignment(Qt::AlignTop);

    auto* lblNext = new QLabel("NEXT PIECES", this);
    lblNext->setStyleSheet("color: #94a1b2; font-size: 11px; font-weight: bold; letter-spacing: 1px;");
    rightPanel->addWidget(lblNext);

    for (int i = 0; i < 3; ++i) {
        auto* prev = new PiecePreviewWidget(QString("#%1").arg(i + 1), defaultPalette, 72, this);
        m_nextPreviews.push_back(prev);
        rightPanel->addWidget(prev);
    }

    // Locked In-Game Match Info Frame (No Mid-Game Modification!)
    auto* optionsFrame = new QFrame(this);
    optionsFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #161b26;"
        "  border: 1px solid #283248;"
        "  border-radius: 8px;"
        "  padding: 6px;"
        "}"
    );
    auto* optionsLayout = new QVBoxLayout(optionsFrame);
    optionsLayout->setContentsMargins(8, 8, 8, 8);
    optionsLayout->setSpacing(6);

    auto* lblLockedHeader = new QLabel("RUN SETTINGS [LOCKED]", this);
    lblLockedHeader->setStyleSheet("color: #94a1b2; font-size: 9px; font-weight: bold; letter-spacing: 1px;");
    optionsLayout->addWidget(lblLockedHeader);

    m_lblLockedElements = new QLabel("Elements: ON | Lvl 1", this);
    m_lblLockedElements->setStyleSheet("color: #68c48a; font-size: 11px; font-weight: bold;");
    optionsLayout->addWidget(m_lblLockedElements);

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(6);

    m_btnSound = new QPushButton("🔊 Sound", this);
    m_btnSound->setStyleSheet(
        "QPushButton {"
        "  background-color: #242c3d;"
        "  color: #f7fafc;"
        "  border: 1px solid #37435f;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #313c54;"
        "  border-color: #63b3ed;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1b212e;"
        "}"
    );
    connect(m_btnSound, &QPushButton::clicked, this, &MainWindow::onToggleSound);
    btnRow->addWidget(m_btnSound);

    m_btnRestart = new QPushButton("🔄 Restart", this);
    m_btnRestart->setStyleSheet(
        "QPushButton {"
        "  background-color: #242c3d;"
        "  color: #f7fafc;"
        "  border: 1px solid #37435f;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #3d3428;"
        "  border-color: #e2a03f;"
        "  color: #f6ad55;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #262118;"
        "}"
    );
    connect(m_btnRestart, &QPushButton::clicked, this, &MainWindow::onRestartGame);
    btnRow->addWidget(m_btnRestart);

    optionsLayout->addLayout(btnRow);

    m_btnMenu = new QPushButton("🏠 Main Menu", this);
    m_btnMenu->setStyleSheet(
        "QPushButton {"
        "  background-color: #2b364c;"
        "  color: #90cdf4;"
        "  border: 1px solid #3e5073;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #364461;"
        "  border-color: #63b3ed;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1e2637;"
        "}"
    );
    connect(m_btnMenu, &QPushButton::clicked, this, &MainWindow::onReturnToMenu);
    optionsLayout->addWidget(m_btnMenu);

    rightPanel->addWidget(optionsFrame);

    m_controlsGuide = new ControlsGuideWidget(this);
    rightPanel->addWidget(m_controlsGuide);
    rightPanel->addStretch();

    // Assemble game layout
    mainLayout->addLayout(leftPanel);
    mainLayout->addLayout(centerPanel);
    mainLayout->addLayout(rightPanel);

    return gameContainer;
}

void MainWindow::onStartGame() {
    const auto& pal = getGamePalette();

    // Lock in settings on canvas
    m_canvas->applySettings(m_settings);
    m_holdPreview->setPalette(pal);
    for (auto* prev : m_nextPreviews) {
        prev->setPalette(pal);
    }

    // Update locked info badges
    m_lblLockedElements->setText(QString("Elements: %1 | Lvl %2").arg(m_settings.enableElements ? "ON" : "OFF").arg(m_settings.startingLevel));

    // Update audio
    m_audio.setMuted(!m_settings.soundEnabled);
    m_audio.setVolume(m_settings.volume);
    m_btnSound->setText(m_settings.soundEnabled ? "🔊 Sound" : "🔇 Muted");

    // Switch to game screen
    m_stack->setCurrentWidget(m_gameScreen);
    m_canvas->restartGame();
    m_canvas->setFocus();
    setFixedSize(sizeHint());
}

void MainWindow::onOpenSettings() {
    m_stack->setCurrentWidget(m_settingsView);
    setFixedSize(sizeHint());
}

void MainWindow::onOpenHowToPlay() {
    m_stack->setCurrentWidget(m_howToPlayView);
    setFixedSize(sizeHint());
}

void MainWindow::onReturnToMenu() {
    if (m_canvas) {
        m_canvas->getEngine().setPaused(true);
    }
    m_mainMenu->updateSettingsBadge(m_settings);
    m_stack->setCurrentWidget(m_mainMenu);
    setFixedSize(sizeHint());
}

void MainWindow::onSettingsChanged(const GameSettings& settings) {
    m_settings = settings;
    m_audio.setMuted(!settings.soundEnabled);
    m_audio.setVolume(settings.volume);
    m_mainMenu->updateSettingsBadge(m_settings);
}

void MainWindow::syncUI() {
    if (!m_canvas || m_stack->currentWidget() != m_gameScreen) return;
    const SandEngine& eng = m_canvas->getEngine();

    m_cardScore->setValue(QLocale().toString(eng.getScore()));
    m_cardHighScore->setValue(QLocale().toString(eng.getHighScore()));
    m_cardLevel->setValue(QString::number(eng.getLevel()));
    m_cardBands->setValue(QString::number(eng.getLinesCleared()));
    m_comboBadge->updateCombo(eng.getCombo());

    m_holdPreview->setPiece(eng.getHoldPiece());

    const auto& nextQ = eng.getNextQueue();
    for (size_t i = 0; i < m_nextPreviews.size(); ++i) {
        if (i < nextQ.size()) {
            m_nextPreviews[i]->setPiece(nextQ[i].get());
        } else {
            m_nextPreviews[i]->setPiece(nullptr);
        }
    }
}

void MainWindow::onToggleSound() {
    bool muted = m_audio.toggleMute();
    m_settings.soundEnabled = !muted;
    m_btnSound->setText(muted ? "🔇 Muted" : "🔊 Sound");
    if (m_canvas) m_canvas->setFocus();
}

void MainWindow::onRestartGame() {
    if (m_canvas) {
        m_canvas->restartGame();
        m_canvas->setFocus();
    }
}
