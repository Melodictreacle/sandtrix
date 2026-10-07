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
    initLayout();

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::syncUI);
    timer->start(30);
}

void MainWindow::initLayout() {
    setStyleSheet(
        "QMainWindow {"
        "  background-color: #0b0d13;"
        "}"
        "QWidget {"
        "  color: #e2e8f0;"
        "  font-family: 'Segoe UI', system-ui, sans-serif;"
        "}"
        "QComboBox {"
        "  background-color: #161822;"
        "  border: 1px solid #2a2e3f;"
        "  border-radius: 6px;"
        "  padding: 6px 12px;"
        "  color: #00E6FF;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 20px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #161822;"
        "  border: 1px solid #2a2e3f;"
        "  selection-background-color: #212534;"
        "  color: #e2e8f0;"
        "}"
        "QPushButton {"
        "  background-color: #1a1e2d;"
        "  border: 1px solid #2a2e3f;"
        "  border-radius: 6px;"
        "  padding: 7px 12px;"
        "  color: #e2e8f0;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #252b3f;"
        "  border-color: #00E6FF;"
        "  color: #00E6FF;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #161822;"
        "}"
    );

    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(16);

    const auto& palettes = getAvailablePalettes();
    const auto& defaultPalette = palettes[0];

    // ----------------- LEFT PANEL -----------------
    auto* leftPanel = new QVBoxLayout();
    leftPanel->setSpacing(10);
    leftPanel->setAlignment(Qt::AlignTop);

    m_holdPreview = new PiecePreviewWidget("HOLD", defaultPalette, 84, this);
    leftPanel->addWidget(m_holdPreview);

    m_cardScore = new StatCard("Score", "0", "#00E6FF", this);
    m_cardHighScore = new StatCard("High Score", "0", "#FFD700", this);
    m_cardLevel = new StatCard("Level", "1", "#2ED573", this);
    m_cardBands = new StatCard("Lines Cleared", "0", "#FF4B82", this);

    leftPanel->addWidget(m_cardScore);
    leftPanel->addWidget(m_cardHighScore);
    leftPanel->addWidget(m_cardLevel);
    leftPanel->addWidget(m_cardBands);

    m_comboBadge = new ComboBadge(this);
    leftPanel->addWidget(m_comboBadge);
    leftPanel->addStretch();

    // ----------------- CENTER PANEL -----------------
    auto* centerPanel = new QVBoxLayout();
    centerPanel->setAlignment(Qt::AlignCenter);

    m_canvas = new GameCanvas(&m_audio, defaultPalette, this);
    centerPanel->addWidget(m_canvas);

    // ----------------- RIGHT PANEL -----------------
    auto* rightPanel = new QVBoxLayout();
    rightPanel->setSpacing(10);
    rightPanel->setAlignment(Qt::AlignTop);

    auto* lblNext = new QLabel("NEXT PIECES", this);
    lblNext->setStyleSheet("color: #7d8597; font-size: 11px; font-weight: bold; letter-spacing: 1px;");
    rightPanel->addWidget(lblNext);

    for (int i = 0; i < 3; ++i) {
        auto* prev = new PiecePreviewWidget(QString("#%1").arg(i + 1), defaultPalette, 72, this);
        m_nextPreviews.push_back(prev);
        rightPanel->addWidget(prev);
    }

    // Options Frame
    auto* optionsFrame = new QFrame(this);
    optionsFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #12141c;"
        "  border: 1px solid #1f2333;"
        "  border-radius: 8px;"
        "  padding: 6px;"
        "}"
    );
    auto* optionsLayout = new QVBoxLayout(optionsFrame);
    optionsLayout->setContentsMargins(8, 8, 8, 8);
    optionsLayout->setSpacing(8);

    auto* lblPal = new QLabel("COLOR PALETTE", this);
    lblPal->setStyleSheet("color: #7d8597; font-size: 10px; font-weight: bold; letter-spacing: 1px;");
    optionsLayout->addWidget(lblPal);

    m_comboPalette = new QComboBox(this);
    for (const auto& p : palettes) {
        m_comboPalette->addItem(p.name);
    }
    connect(m_comboPalette, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onPaletteChanged);
    optionsLayout->addWidget(m_comboPalette);

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(6);

    m_btnSound = new QPushButton("🔊 Sound", this);
    connect(m_btnSound, &QPushButton::clicked, this, &MainWindow::onToggleSound);
    btnRow->addWidget(m_btnSound);

    m_btnRestart = new QPushButton("🔄 Restart", this);
    connect(m_btnRestart, &QPushButton::clicked, this, &MainWindow::onRestartGame);
    btnRow->addWidget(m_btnRestart);

    optionsLayout->addLayout(btnRow);
    rightPanel->addWidget(optionsFrame);

    m_controlsGuide = new ControlsGuideWidget(this);
    rightPanel->addWidget(m_controlsGuide);
    rightPanel->addStretch();

    // Assemble main layout
    mainLayout->addLayout(leftPanel);
    mainLayout->addLayout(centerPanel);
    mainLayout->addLayout(rightPanel);

    setCentralWidget(centralWidget);
    setFixedSize(sizeHint());
}

void MainWindow::syncUI() {
    if (!m_canvas) return;
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

void MainWindow::onPaletteChanged(int index) {
    const auto& palettes = getAvailablePalettes();
    if (index >= 0 && index < static_cast<int>(palettes.size())) {
        const auto& pal = palettes[index];
        m_canvas->setPalette(pal);
        m_holdPreview->setPalette(pal);
        for (auto* prev : m_nextPreviews) {
            prev->setPalette(pal);
        }
        m_canvas->setFocus();
    }
}

void MainWindow::onToggleSound() {
    bool muted = m_audio.toggleMute();
    m_btnSound->setText(muted ? "🔇 Muted" : "🔊 Sound");
    m_canvas->setFocus();
}

void MainWindow::onRestartGame() {
    m_canvas->restartGame();
    m_canvas->setFocus();
}
