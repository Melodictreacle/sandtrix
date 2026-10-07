#include "MenuWidgets.h"
#include <QFrame>
#include <QScrollArea>
#include <QCoreApplication>
#include <QFont>

// ==================== MainMenuWidget ====================

MainMenuWidget::MainMenuWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(20);

    // Title Box
    auto* titleBox = new QVBoxLayout();
    titleBox->setAlignment(Qt::AlignCenter);
    titleBox->setSpacing(4);

    auto* lblTitle = new QLabel("SANDTRIX", this);
    lblTitle->setAlignment(Qt::AlignCenter);
    lblTitle->setStyleSheet(
        "color: #00E6FF;"
        "font-size: 46px;"
        "font-weight: 900;"
        "letter-spacing: 4px;"
        "font-family: 'Segoe UI', system-ui, sans-serif;"
    );

    auto* lblSubtitle = new QLabel("SAND TETRIS FOR QT", this);
    lblSubtitle->setAlignment(Qt::AlignCenter);
    lblSubtitle->setStyleSheet(
        "color: #7d8597;"
        "font-size: 13px;"
        "font-weight: bold;"
        "letter-spacing: 2px;"
    );

    m_lblBadge = new QLabel(this);
    m_lblBadge->setAlignment(Qt::AlignCenter);
    m_lblBadge->setStyleSheet(
        "background-color: #161822;"
        "color: #a0aec0;"
        "font-size: 11px;"
        "padding: 6px 14px;"
        "border-radius: 12px;"
        "border: 1px solid #2a2e3f;"
    );

    titleBox->addWidget(lblTitle);
    titleBox->addWidget(lblSubtitle);
    titleBox->addSpacing(8);
    titleBox->addWidget(m_lblBadge);

    mainLayout->addLayout(titleBox);
    mainLayout->addSpacing(15);

    // Menu Buttons Frame
    auto* menuCard = new QFrame(this);
    menuCard->setFixedWidth(320);
    menuCard->setStyleSheet(
        "QFrame {"
        "  background-color: #12141c;"
        "  border: 1px solid #202434;"
        "  border-radius: 12px;"
        "  padding: 16px;"
        "}"
    );

    auto* menuLayout = new QVBoxLayout(menuCard);
    menuLayout->setSpacing(12);

    auto* btnStart = new QPushButton("▶  START GAME", this);
    btnStart->setFixedHeight(46);
    btnStart->setStyleSheet(
        "QPushButton {"
        "  background-color: #00E6FF;"
        "  color: #0b0d13;"
        "  font-size: 15px;"
        "  font-weight: 900;"
        "  border-radius: 8px;"
        "  letter-spacing: 1px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #33ecff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #00b3c7;"
        "}"
    );
    connect(btnStart, &QPushButton::clicked, this, &MainMenuWidget::startClicked);

    auto* btnSettings = new QPushButton("⚙  SETTINGS", this);
    btnSettings->setFixedHeight(40);
    btnSettings->setStyleSheet(
        "QPushButton {"
        "  background-color: #1a1e2d;"
        "  color: #e2e8f0;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  border: 1px solid #2a2e3f;"
        "  border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #262c42;"
        "  border-color: #00E6FF;"
        "  color: #00E6FF;"
        "}"
    );
    connect(btnSettings, &QPushButton::clicked, this, &MainMenuWidget::settingsClicked);

    auto* btnHowToPlay = new QPushButton("📖  HOW TO PLAY", this);
    btnHowToPlay->setFixedHeight(40);
    btnHowToPlay->setStyleSheet(
        "QPushButton {"
        "  background-color: #1a1e2d;"
        "  color: #e2e8f0;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  border: 1px solid #2a2e3f;"
        "  border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #262c42;"
        "  border-color: #FF4B82;"
        "  color: #FF4B82;"
        "}"
    );
    connect(btnHowToPlay, &QPushButton::clicked, this, &MainMenuWidget::howToPlayClicked);

    auto* btnExit = new QPushButton("✕  EXIT", this);
    btnExit->setFixedHeight(36);
    btnExit->setStyleSheet(
        "QPushButton {"
        "  background-color: transparent;"
        "  color: #718096;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  color: #e53e3e;"
        "}"
    );
    connect(btnExit, &QPushButton::clicked, this, &MainMenuWidget::exitClicked);

    menuLayout->addWidget(btnStart);
    menuLayout->addWidget(btnSettings);
    menuLayout->addWidget(btnHowToPlay);
    menuLayout->addWidget(btnExit);

    mainLayout->addWidget(menuCard, 0, Qt::AlignCenter);
    mainLayout->addSpacing(10);
}

void MainMenuWidget::updateSettingsBadge(const GameSettings& settings) {
    const auto& palettes = getAvailablePalettes();
    QString palName = (settings.paletteIndex < static_cast<int>(palettes.size())) ? palettes[settings.paletteIndex].name : "Custom";
    QString elemStr = settings.enableElements ? "🌊 Elements ON" : "Elements OFF";
    QString sndStr = settings.soundEnabled ? "🔊 Sound ON" : "🔇 Sound OFF";

    m_lblBadge->setText(QString("⚙ Preset: [%1]  •  Level %2  •  %3  •  %4")
        .arg(palName).arg(settings.startingLevel).arg(elemStr).arg(sndStr));
}

// ==================== SettingsWidget ====================

SettingsWidget::SettingsWidget(const GameSettings& currentSettings, QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(16);

    auto* header = new QLabel("⚙ GAME SETTINGS", this);
    header->setStyleSheet("color: #00E6FF; font-size: 26px; font-weight: 900; letter-spacing: 2px;");
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    auto* notice = new QLabel("Note: Settings are locked once you start the game to ensure competitive fairness.", this);
    notice->setStyleSheet("color: #718096; font-size: 11px;");
    notice->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(notice);

    auto* card = new QFrame(this);
    card->setFixedWidth(440);
    card->setStyleSheet(
        "QFrame {"
        "  background-color: #12141c;"
        "  border: 1px solid #202434;"
        "  border-radius: 12px;"
        "  padding: 18px;"
        "}"
    );
    auto* formLayout = new QVBoxLayout(card);
    formLayout->setSpacing(14);

    // 1. Palette Option
    auto* lblPalTitle = new QLabel("COLOR PALETTE", this);
    lblPalTitle->setStyleSheet("color: #7d8597; font-size: 11px; font-weight: bold;");
    m_comboPalette = new QComboBox(this);
    const auto& palettes = getAvailablePalettes();
    for (const auto& p : palettes) {
        m_comboPalette->addItem(p.name);
    }
    m_comboPalette->setCurrentIndex(currentSettings.paletteIndex);

    m_lblPaletteDesc = new QLabel(this);
    m_lblPaletteDesc->setStyleSheet("color: #00E6FF; font-size: 11px; font-style: italic;");
    connect(m_comboPalette, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::onPaletteIndexChanged);
    onPaletteIndexChanged(currentSettings.paletteIndex);

    formLayout->addWidget(lblPalTitle);
    formLayout->addWidget(m_comboPalette);
    formLayout->addWidget(m_lblPaletteDesc);

    // 2. Starting Level Option
    auto* lblLevelTitle = new QLabel("STARTING SPEED / LEVEL (1 - 10)", this);
    lblLevelTitle->setStyleSheet("color: #7d8597; font-size: 11px; font-weight: bold;");
    m_spinLevel = new QSpinBox(this);
    m_spinLevel->setRange(1, 10);
    m_spinLevel->setValue(currentSettings.startingLevel);
    m_spinLevel->setStyleSheet(
        "QSpinBox {"
        "  background-color: #161822;"
        "  border: 1px solid #2a2e3f;"
        "  border-radius: 6px;"
        "  padding: 6px 12px;"
        "  color: #2ED573;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
    );

    formLayout->addWidget(lblLevelTitle);
    formLayout->addWidget(m_spinLevel);

    // 3. Elemental Materials Option
    m_chkElements = new QCheckBox("Enable Elemental Pieces (🌊 Water & ☣ Acid)", this);
    m_chkElements->setChecked(currentSettings.enableElements);
    m_chkElements->setStyleSheet(
        "QCheckBox {"
        "  color: #e2e8f0;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "}"
        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "}"
    );
    auto* elemDesc = new QLabel("  • Water flows flat and floats over sand (Tidal Waves!)\n  • Acid dissolves sand directly beneath it on contact", this);
    elemDesc->setStyleSheet("color: #718096; font-size: 10px;");

    formLayout->addWidget(m_chkElements);
    formLayout->addWidget(elemDesc);

    // 4. Sound & Volume Option
    m_chkSound = new QCheckBox("Enable Sound Effects", this);
    m_chkSound->setChecked(currentSettings.soundEnabled);
    m_chkSound->setStyleSheet("color: #e2e8f0; font-size: 12px; font-weight: bold;");

    m_sliderVolume = new QSlider(Qt::Horizontal, this);
    m_sliderVolume->setRange(0, 100);
    m_sliderVolume->setValue(static_cast<int>(currentSettings.volume * 100.0f));

    formLayout->addWidget(m_chkSound);
    formLayout->addWidget(m_sliderVolume);

    mainLayout->addWidget(card, 0, Qt::AlignCenter);

    // Save & Return Button
    auto* btnSave = new QPushButton("💾  SAVE & RETURN", this);
    btnSave->setFixedSize(200, 40);
    btnSave->setStyleSheet(
        "QPushButton {"
        "  background-color: #00E6FF;"
        "  color: #0b0d13;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #33ecff;"
        "}"
    );
    connect(btnSave, &QPushButton::clicked, this, &SettingsWidget::onSaveClicked);

    mainLayout->addWidget(btnSave, 0, Qt::AlignCenter);
}

void SettingsWidget::onPaletteIndexChanged(int index) {
    const auto& palettes = getAvailablePalettes();
    if (index >= 0 && index < static_cast<int>(palettes.size())) {
        m_lblPaletteDesc->setText("Description: " + palettes[index].description);
    }
}

GameSettings SettingsWidget::getSettings() const {
    GameSettings s;
    s.paletteIndex = m_comboPalette->currentIndex();
    s.startingLevel = m_spinLevel->value();
    s.enableElements = m_chkElements->isChecked();
    s.soundEnabled = m_chkSound->isChecked();
    s.volume = m_sliderVolume->value() / 100.0f;
    return s;
}

void SettingsWidget::onSaveClicked() {
    emit settingsChanged(getSettings());
    emit backClicked();
}

// ==================== HowToPlayWidget ====================

HowToPlayWidget::HowToPlayWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(40, 20, 40, 20);
    mainLayout->setSpacing(14);

    auto* header = new QLabel("📖 HOW TO PLAY SANDTRIX", this);
    header->setStyleSheet("color: #FF4B82; font-size: 26px; font-weight: 900; letter-spacing: 2px;");
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setFixedWidth(560);
    scrollArea->setFixedHeight(480);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea {"
        "  border: 1px solid #202434;"
        "  background-color: #12141c;"
        "  border-radius: 12px;"
        "}"
    );

    auto* contentWidget = new QWidget();
    contentWidget->setStyleSheet("background-color: transparent;");
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(18, 18, 18, 18);
    contentLayout->setSpacing(14);

    auto addCard = [&](const QString& title, const QString& body, const QString& color) {
        auto* card = new QFrame(contentWidget);
        card->setStyleSheet(
            "QFrame {"
            "  background-color: #161822;"
            "  border: 1px solid #2a2e3f;"
            "  border-radius: 8px;"
            "  padding: 10px;"
            "}"
        );
        auto* cl = new QVBoxLayout(card);
        cl->setSpacing(4);

        auto* t = new QLabel(title, card);
        t->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold;").arg(color));

        auto* b = new QLabel(body, card);
        b->setStyleSheet("color: #cbd5e0; font-size: 11px; line-height: 1.4;");
        b->setWordWrap(true);

        cl->addWidget(t);
        cl->addWidget(b);
        contentLayout->addWidget(card);
    };

    addCard("⏳ 1. The Sand Dissolution Rule",
            "Blocks fall as classic Tetris tetrominoes (I, O, T, S, Z, J, L) that you can rotate, steer, and hold. "
            "However, the instant a block hits the ground or resting sand, it dissolves into hundreds of loose sand grains!",
            "#00E6FF");

    addCard("⚡ 2. Edge-to-Edge Line Clears",
            "Unlike traditional Tetris where straight horizontal rows clear, in Sandtrix you clear lines by forming an "
            "unbroken path of the SAME COLOR connecting the LEFT WALL (x=0) to the RIGHT WALL (x=W-1). "
            "Any 8-way continuous connected path of that color vaporizes!",
            "#FFD700");

    addCard("🔥 3. Avalanche Combos",
            "When a colored band clears, all sand resting on top avalanches down into the gap. "
            "If the collapsing cascade creates a NEW connection between the walls, a COMBO CLEAR triggers with exponential score multipliers!",
            "#FF4B82");

    addCard("🌊 4. Elemental Pieces",
            "• Water Blocks: Liquid flows horizontally to fill gaps flat. Heavy sand sinks through water! Connecting water edge-to-edge triggers a Tidal Wave (+800 pts)!\n"
            "• Acid Blocks: Radioactive green blocks that actively dissolve sand directly beneath them on impact.",
            "#2ED573");

    addCard("🎮 5. Complete Keyboard Controls",
            "• [← / →] or [A / D] : Move Left / Right (DAS & ARR enabled)\n"
            "• [↑] or [W / X] : Rotate Clockwise\n"
            "• [Z / Ctrl] : Rotate Counter-Clockwise\n"
            "• [↓ / S] : Soft Drop\n"
            "• [SPACE] : Hard Drop\n"
            "• [C / Shift] : Hold Piece\n"
            "• [P / Esc] : Pause / Resume\n"
            "• [R] : Restart Run\n"
            "• [M] : Mute Sound",
            "#A259FF");

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea, 0, Qt::AlignCenter);

    auto* btnBack = new QPushButton("⬅  BACK TO MENU", this);
    btnBack->setFixedSize(180, 38);
    btnBack->setStyleSheet(
        "QPushButton {"
        "  background-color: #1a1e2d;"
        "  color: #e2e8f0;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "  border: 1px solid #2a2e3f;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #262c42;"
        "  border-color: #FF4B82;"
        "  color: #FF4B82;"
        "}"
    );
    connect(btnBack, &QPushButton::clicked, this, &HowToPlayWidget::backClicked);

    mainLayout->addWidget(btnBack, 0, Qt::AlignCenter);
}
