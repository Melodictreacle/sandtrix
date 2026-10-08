#include "MainWindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QTimer>
#include <QShortcut>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_canvas(nullptr)
{
    setWindowTitle("SANDTRIX - Sand Tetris for Qt");
    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    initStackedViews();

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::syncUI);
    timer->start(30);

    // Global keyboard shortcuts for Fullscreen toggle:
    auto* shortcutF11 = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(shortcutF11, &QShortcut::activated, this, &MainWindow::toggleFullScreen);

    auto* shortcutAltEnter = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Return), this);
    connect(shortcutAltEnter, &QShortcut::activated, this, &MainWindow::toggleFullScreen);
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
    connect(m_mainMenu, &MainMenuWidget::modeToggled, this, &MainWindow::onToggleMode);
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
    switchToScreen(m_mainMenu);
}

void MainWindow::switchToScreen(QWidget* screen) {
    if (!screen) return;
    m_stack->setCurrentWidget(screen);
    QSize target = screen->sizeHint();
    if (target.width() < 420) target.setWidth(420);
    if (target.height() < 460) target.setHeight(460);

    setMinimumSize(target);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    if (!isFullScreen() && !isMaximized()) {
        resize(target);
    }
}

QWidget* MainWindow::createGameScreen() {
    auto* outerScreen = new QWidget(this);
    auto* outerLayout = new QVBoxLayout(outerScreen);
    outerLayout->setAlignment(Qt::AlignCenter);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto* gameContainer = new QWidget(outerScreen);
    auto* mainLayout = new QHBoxLayout(gameContainer);
    mainLayout->setContentsMargins(28, 20, 28, 20);
    mainLayout->setSpacing(32); // Spacious 32px gap between panels and the main game box!

    const auto& defaultPalette = getGamePalette();

    // ----------------- LEFT PANEL (190px width) -----------------
    auto* leftContainer = new QWidget(gameContainer);
    leftContainer->setFixedWidth(190);
    auto* leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);
    leftLayout->setAlignment(Qt::AlignTop);

    m_holdPreview = new PiecePreviewWidget("HOLD", defaultPalette, 84, this);
    leftLayout->addWidget(m_holdPreview, 0, Qt::AlignCenter);

    m_cardScore = new StatCard("Score", "0", "#327DEB", this);
    m_cardHighScore = new StatCard("High Score", "0", "#F5B923", this);
    m_cardLevel = new StatCard("Level", "1", "#28B964", this);
    m_cardBands = new StatCard("Lines Cleared", "0", "#EB4B4B", this);

    leftLayout->addWidget(m_cardScore);
    leftLayout->addWidget(m_cardHighScore);
    leftLayout->addWidget(m_cardLevel);
    leftLayout->addWidget(m_cardBands);

    m_comboBadge = new ComboBadge(this);
    leftLayout->addWidget(m_comboBadge);
    leftLayout->addStretch();

    // ----------------- CENTER CANVAS: MAIN GAME BOX (408px width) -----------------
    auto* centerContainer = new QWidget(gameContainer);
    centerContainer->setFixedWidth(408);
    auto* centerLayout = new QVBoxLayout(centerContainer);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setAlignment(Qt::AlignCenter);

    auto* canvasFrame = new QFrame(centerContainer);
    canvasFrame->setObjectName("canvasFrame");
    canvasFrame->setStyleSheet(
        "QFrame#canvasFrame {"
        "  background-color: #0d1017;"
        "  border: 2px solid #283248;"
        "  border-radius: 8px;"
        "}"
    );
    auto* canvasFrameLayout = new QVBoxLayout(canvasFrame);
    canvasFrameLayout->setContentsMargins(2, 2, 2, 2);
    canvasFrameLayout->setSpacing(0);
    canvasFrameLayout->setAlignment(Qt::AlignCenter);

    m_canvas = new GameCanvas(&m_audio, defaultPalette, canvasFrame);
    canvasFrameLayout->addWidget(m_canvas);
    centerLayout->addWidget(canvasFrame);

    // ----------------- RIGHT PANEL (200px width) -----------------
    auto* rightContainer = new QWidget(gameContainer);
    rightContainer->setFixedWidth(200);
    auto* rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);
    rightLayout->setAlignment(Qt::AlignTop);

    // 1. Next Pieces Box
    auto* nextFrame = new QFrame(rightContainer);
    nextFrame->setObjectName("nextFrame");
    nextFrame->setStyleSheet(
        "QFrame#nextFrame {"
        "  background-color: #161b26;"
        "  border: 1px solid #283248;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "}"
    );
    auto* nextLayout = new QVBoxLayout(nextFrame);
    nextLayout->setContentsMargins(6, 6, 6, 6);
    nextLayout->setSpacing(6);
    nextLayout->setAlignment(Qt::AlignCenter);

    auto* lblNext = new QLabel("NEXT PIECES", nextFrame);
    lblNext->setStyleSheet("color: #94a1b2; font-size: 10px; font-weight: bold; letter-spacing: 1px;");
    lblNext->setAlignment(Qt::AlignCenter);
    nextLayout->addWidget(lblNext);

    for (int i = 0; i < 3; ++i) {
        auto* prev = new PiecePreviewWidget(QString("#%1").arg(i + 1), defaultPalette, 62, this);
        m_nextPreviews.push_back(prev);
        nextLayout->addWidget(prev, 0, Qt::AlignCenter);
    }
    rightLayout->addWidget(nextFrame);

    // 2. In-Game Settings Box
    auto* optionsFrame = new QFrame(rightContainer);
    optionsFrame->setObjectName("optionsFrame");
    optionsFrame->setStyleSheet(
        "QFrame#optionsFrame {"
        "  background-color: #161b26;"
        "  border: 1px solid #283248;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "}"
    );
    auto* optionsLayout = new QVBoxLayout(optionsFrame);
    optionsLayout->setContentsMargins(8, 6, 8, 6);
    optionsLayout->setSpacing(6);

    auto* lblLockedHeader = new QLabel("RUN SETTINGS", optionsFrame);
    lblLockedHeader->setStyleSheet("color: #94a1b2; font-size: 9px; font-weight: bold; letter-spacing: 1px;");
    lblLockedHeader->setAlignment(Qt::AlignCenter);
    optionsLayout->addWidget(lblLockedHeader);

    m_lblLockedElements = new QLabel("Elements: ON | Lvl 1", optionsFrame);
    m_lblLockedElements->setStyleSheet("color: #68c48a; font-size: 11px; font-weight: bold;");
    m_lblLockedElements->setAlignment(Qt::AlignCenter);
    optionsLayout->addWidget(m_lblLockedElements);

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(6);

    m_btnSound = new QPushButton("🔊 Sound", optionsFrame);
    m_btnSound->setStyleSheet(
        "QPushButton {"
        "  background-color: #242c3d;"
        "  color: #f7fafc;"
        "  border: 1px solid #37435f;"
        "  border-radius: 6px;"
        "  padding: 6px 8px;"
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

    m_btnRestart = new QPushButton("🔄 Restart", optionsFrame);
    m_btnRestart->setStyleSheet(
        "QPushButton {"
        "  background-color: #242c3d;"
        "  color: #f7fafc;"
        "  border: 1px solid #37435f;"
        "  border-radius: 6px;"
        "  padding: 6px 8px;"
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

    m_btnMenu = new QPushButton("🏠 Main Menu", optionsFrame);
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

    rightLayout->addWidget(optionsFrame);

    // 3. Controls Box
    m_controlsGuide = new ControlsGuideWidget(rightContainer);
    rightLayout->addWidget(m_controlsGuide);
    rightLayout->addStretch();

    // Assemble game layout with generous side-panel separation
    mainLayout->addWidget(leftContainer);
    mainLayout->addWidget(centerContainer);
    mainLayout->addWidget(rightContainer);

    gameContainer->setFixedSize(918, 748);
    outerLayout->addWidget(gameContainer, 0, Qt::AlignCenter);
    return outerScreen;
}

void MainWindow::toggleFullScreen() {
    if (isFullScreen()) {
        m_settings.fullscreen = false;
        showNormal();
        QWidget* cur = m_stack->currentWidget();
        QSize target = cur ? cur->sizeHint() : sizeHint();
        if (target.width() < 420) target.setWidth(420);
        if (target.height() < 460) target.setHeight(460);
        setMinimumSize(target);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        resize(target);
    } else {
        m_settings.fullscreen = true;
        setMinimumSize(0, 0);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        showFullScreen();
    }
    if (m_settingsView) {
        m_settingsView->loadSettings(m_settings);
    }
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
    QString modeName;
    if (m_settings.gameMode == GameMode::ClassicTetris) {
        modeName = "🧱 Classic Tetris";
    } else {
        modeName = m_settings.enableElements ? "⏳ Sandtrix (🌊 Elements: ON)" : "⏳ Sandtrix (Elements: OFF)";
    }
    m_lblLockedElements->setText(QString("%1 | Lvl %2").arg(modeName).arg(m_settings.startingLevel));

    // Update audio
    m_audio.setMuted(!m_settings.soundEnabled);
    m_audio.setVolume(m_settings.volume);
    m_btnSound->setText(m_settings.soundEnabled ? "🔊 Sound" : "🔇 Muted");

    // Switch to game screen
    switchToScreen(m_gameScreen);
    m_canvas->restartGame();
    m_canvas->setFocus();
}

void MainWindow::onOpenSettings() {
    m_settingsView->loadSettings(m_settings);
    switchToScreen(m_settingsView);
}

void MainWindow::onToggleMode() {
    if (m_settings.gameMode == GameMode::Sandtrix) {
        if (!m_settings.enableElements) {
            m_settings.enableElements = true;
        } else {
            m_settings.gameMode = GameMode::ClassicTetris;
            m_settings.enableElements = false;
        }
    } else {
        m_settings.gameMode = GameMode::Sandtrix;
        m_settings.enableElements = false;
    }
    m_mainMenu->updateSettingsBadge(m_settings);
    if (m_settingsView) {
        m_settingsView->loadSettings(m_settings);
    }
}

void MainWindow::onOpenHowToPlay() {
    switchToScreen(m_howToPlayView);
}

void MainWindow::onReturnToMenu() {
    if (m_canvas) {
        m_canvas->getEngine().setPaused(true);
    }
    m_mainMenu->updateSettingsBadge(m_settings);
    switchToScreen(m_mainMenu);
}

void MainWindow::onSettingsChanged(const GameSettings& settings) {
    m_settings = settings;
    m_audio.setMuted(!settings.soundEnabled);
    m_audio.setVolume(settings.volume);
    m_mainMenu->updateSettingsBadge(m_settings);
    if (settings.fullscreen != isFullScreen()) {
        toggleFullScreen();
    }
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
