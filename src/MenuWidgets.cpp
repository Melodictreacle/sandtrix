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
        "color: #7cb7ea;"
        "font-size: 46px;"
        "font-weight: 900;"
        "letter-spacing: 4px;"
        "font-family: 'Segoe UI', system-ui, sans-serif;"
    );

    auto* lblSubtitle = new QLabel("SAND TETRIS FOR QT", this);
    lblSubtitle->setAlignment(Qt::AlignCenter);
    lblSubtitle->setStyleSheet(
        "color: #94a1b2;"
        "font-size: 13px;"
        "font-weight: bold;"
        "letter-spacing: 2px;"
    );

    m_lblBadge = new QLabel(this);
    m_lblBadge->setAlignment(Qt::AlignCenter);
    m_lblBadge->setStyleSheet(
        "background-color: #1a202c;"
        "color: #cbd5e0;"
        "font-size: 11px;"
        "font-weight: 500;"
        "padding: 6px 14px;"
        "border-radius: 12px;"
        "border: 1px solid #2d3748;"
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
        "  background-color: #161b26;"
        "  border: 1px solid #283248;"
        "  border-radius: 14px;"
        "  padding: 18px;"
        "}"
    );

    auto* menuLayout = new QVBoxLayout(menuCard);
    menuLayout->setSpacing(12);

    auto* btnStart = new QPushButton("▶  START GAME", this);
    btnStart->setFixedHeight(46);
    btnStart->setStyleSheet(
        "QPushButton {"
        "  background-color: #2b7a78;"
        "  color: #ffffff;"
        "  font-size: 15px;"
        "  font-weight: 800;"
        "  border: 1px solid #3d9996;"
        "  border-radius: 8px;"
        "  letter-spacing: 1px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #389592;"
        "  border-color: #55b7b4;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #205c5a;"
        "}"
    );
    connect(btnStart, &QPushButton::clicked, this, &MainMenuWidget::startClicked);

    auto* btnSettings = new QPushButton("⚙  SETTINGS", this);
    btnSettings->setFixedHeight(42);
    btnSettings->setStyleSheet(
        "QPushButton {"
        "  background-color: #242c3d;"
        "  color: #f7fafc;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  border: 1px solid #37435f;"
        "  border-radius: 8px;"
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
    connect(btnSettings, &QPushButton::clicked, this, &MainMenuWidget::settingsClicked);

    auto* btnHowToPlay = new QPushButton("📖  HOW TO PLAY", this);
    btnHowToPlay->setFixedHeight(42);
    btnHowToPlay->setStyleSheet(
        "QPushButton {"
        "  background-color: #242c3d;"
        "  color: #f7fafc;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  border: 1px solid #37435f;"
        "  border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #313c54;"
        "  border-color: #e27d9a;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1b212e;"
        "}"
    );
    connect(btnHowToPlay, &QPushButton::clicked, this, &MainMenuWidget::howToPlayClicked);

    auto* btnExit = new QPushButton("✕  EXIT GAME", this);
    btnExit->setFixedHeight(40);
    btnExit->setStyleSheet(
        "QPushButton {"
        "  background-color: #242c3d;"
        "  color: #e57373;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  border: 1px solid #4a343e;"
        "  border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #4a232c;"
        "  border-color: #c54f60;"
        "  color: #ffb3be;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #35181f;"
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
    header->setStyleSheet("color: #7cb7ea; font-size: 26px; font-weight: 900; letter-spacing: 2px;");
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    auto* notice = new QLabel("Note: Settings are locked once you start the game to ensure competitive fairness.", this);
    notice->setStyleSheet("color: #94a1b2; font-size: 11px;");
    notice->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(notice);

    auto* card = new QFrame(this);
    card->setFixedWidth(460);
    card->setStyleSheet(
        "QFrame {"
        "  background-color: #161b26;"
        "  border: 1px solid #283248;"
        "  border-radius: 14px;"
        "  padding: 18px;"
        "}"
    );
    auto* formLayout = new QVBoxLayout(card);
    formLayout->setSpacing(14);

    // 1. Palette Option
    auto* lblPalTitle = new QLabel("COLOR PALETTE", this);
    lblPalTitle->setStyleSheet("color: #94a1b2; font-size: 11px; font-weight: bold; letter-spacing: 1px;");
    m_comboPalette = new QComboBox(this);
    m_comboPalette->setStyleSheet(
        "QComboBox {"
        "  background-color: #242c3d;"
        "  border: 1px solid #37435f;"
        "  border-radius: 6px;"
        "  padding: 7px 12px;"
        "  color: #f7fafc;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QComboBox:hover {"
        "  border-color: #63b3ed;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #1a202c;"
        "  color: #f7fafc;"
        "  selection-background-color: #2b7a78;"
        "  selection-color: #ffffff;"
        "  border: 1px solid #37435f;"
        "  padding: 4px;"
        "}"
    );
    const auto& palettes = getAvailablePalettes();
    for (const auto& p : palettes) {
        m_comboPalette->addItem(p.name);
    }
    m_comboPalette->setCurrentIndex(currentSettings.paletteIndex);

    m_lblPaletteDesc = new QLabel(this);
    m_lblPaletteDesc->setStyleSheet("color: #90cdf4; font-size: 11px; font-style: italic;");
    connect(m_comboPalette, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::onPaletteIndexChanged);
    onPaletteIndexChanged(currentSettings.paletteIndex);

    formLayout->addWidget(lblPalTitle);
    formLayout->addWidget(m_comboPalette);
    formLayout->addWidget(m_lblPaletteDesc);

    // 2. Starting Level Option
    auto* lblLevelTitle = new QLabel("STARTING SPEED / LEVEL (1 - 10)", this);
    lblLevelTitle->setStyleSheet("color: #94a1b2; font-size: 11px; font-weight: bold; letter-spacing: 1px;");
    m_spinLevel = new QSpinBox(this);
    m_spinLevel->setRange(1, 10);
    m_spinLevel->setValue(currentSettings.startingLevel);
    m_spinLevel->setStyleSheet(
        "QSpinBox {"
        "  background-color: #242c3d;"
        "  border: 1px solid #37435f;"
        "  border-radius: 6px;"
        "  padding: 6px 12px;"
        "  color: #68c48a;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QSpinBox:hover {"
        "  border-color: #68c48a;"
        "}"
    );

    formLayout->addWidget(lblLevelTitle);
    formLayout->addWidget(m_spinLevel);

    // 3. Elemental Materials Option
    m_chkElements = new QCheckBox("Enable Elemental Pieces (🌊 Water & ☣ Acid)", this);
    m_chkElements->setChecked(currentSettings.enableElements);
    m_chkElements->setStyleSheet(
        "QCheckBox {"
        "  color: #f7fafc;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "}"
        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "}"
    );
    auto* elemDesc = new QLabel("  • Water flows flat and floats over sand (Tidal Waves!)\n  • Acid dissolves sand directly beneath it on contact", this);
    elemDesc->setStyleSheet("color: #94a1b2; font-size: 10px;");

    formLayout->addWidget(m_chkElements);
    formLayout->addWidget(elemDesc);

    // 4. Sound & Volume Option
    m_chkSound = new QCheckBox("Enable Sound Effects", this);
    m_chkSound->setChecked(currentSettings.soundEnabled);
    m_chkSound->setStyleSheet(
        "QCheckBox {"
        "  color: #f7fafc;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "}"
        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "}"
    );

    m_sliderVolume = new QSlider(Qt::Horizontal, this);
    m_sliderVolume->setRange(0, 100);
    m_sliderVolume->setValue(static_cast<int>(currentSettings.volume * 100.0f));
    m_sliderVolume->setStyleSheet(
        "QSlider::groove:horizontal {"
        "  height: 8px;"
        "  background: #242c3d;"
        "  border: 1px solid #37435f;"
        "  border-radius: 4px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: #2b7a78;"
        "  border-radius: 4px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: #55b7b4;"
        "  border: 1px solid #ffffff;"
        "  width: 16px;"
        "  margin-top: -5px;"
        "  margin-bottom: -5px;"
        "  border-radius: 8px;"
        "}"
    );

    formLayout->addWidget(m_chkSound);
    formLayout->addWidget(m_sliderVolume);

    mainLayout->addWidget(card, 0, Qt::AlignCenter);

    // Button Row: Save & Return + Cancel
    auto* btnRowLayout = new QHBoxLayout();
    btnRowLayout->setSpacing(12);

    auto* btnSave = new QPushButton("💾  SAVE & RETURN", this);
    btnSave->setFixedSize(170, 42);
    btnSave->setStyleSheet(
        "QPushButton {"
        "  background-color: #2b7a78;"
        "  color: #ffffff;"
        "  font-size: 13px;"
        "  font-weight: 800;"
        "  border: 1px solid #3d9996;"
        "  border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #389592;"
        "  border-color: #55b7b4;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #205c5a;"
        "}"
    );
    connect(btnSave, &QPushButton::clicked, this, &SettingsWidget::onSaveClicked);

    auto* btnCancel = new QPushButton("⬅  CANCEL", this);
    btnCancel->setFixedSize(140, 42);
    btnCancel->setStyleSheet(
        "QPushButton {"
        "  background-color: #242c3d;"
        "  color: #f7fafc;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  border: 1px solid #37435f;"
        "  border-radius: 8px;"
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
    connect(btnCancel, &QPushButton::clicked, this, &SettingsWidget::backClicked);

    btnRowLayout->addWidget(btnSave);
    btnRowLayout->addWidget(btnCancel);

    mainLayout->addLayout(btnRowLayout);
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
    header->setStyleSheet("color: #7cb7ea; font-size: 26px; font-weight: 900; letter-spacing: 2px;");
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setFixedWidth(560);
    scrollArea->setFixedHeight(480);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea {"
        "  border: 1px solid #283248;"
        "  background-color: #161b26;"
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
            "  background-color: #1e2535;"
            "  border: 1px solid #2d384e;"
            "  border-radius: 8px;"
            "  padding: 12px;"
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
            "#7cb7ea");

    addCard("⚡ 2. Edge-to-Edge Line Clears",
            "Unlike traditional Tetris where straight horizontal rows clear, in Sandtrix you clear lines by forming an "
            "unbroken path of the SAME COLOR connecting the LEFT WALL (x=0) to the RIGHT WALL (x=W-1). "
            "Any 8-way continuous connected path of that color vaporizes!",
            "#e2b755");

    addCard("🔥 3. Avalanche Combos",
            "When a colored band clears, all sand resting on top avalanches down into the gap. "
            "If the collapsing cascade creates a NEW connection between the walls, a COMBO CLEAR triggers with exponential score multipliers!",
            "#e27d9a");

    addCard("🌊 4. Elemental Pieces",
            "• Water Blocks: Liquid flows horizontally to fill gaps flat. Heavy sand sinks through water! Connecting water edge-to-edge triggers a Tidal Wave (+800 pts)!\n"
            "• Acid Blocks: Soft jade blocks that actively dissolve sand directly beneath them on impact.",
            "#68c48a");

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
            "#b89bf0");

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea, 0, Qt::AlignCenter);

    auto* btnBack = new QPushButton("⬅  BACK TO MENU", this);
    btnBack->setFixedSize(200, 42);
    btnBack->setStyleSheet(
        "QPushButton {"
        "  background-color: #242c3d;"
        "  color: #f7fafc;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  border: 1px solid #37435f;"
        "  border-radius: 8px;"
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
    connect(btnBack, &QPushButton::clicked, this, &HowToPlayWidget::backClicked);

    mainLayout->addWidget(btnBack, 0, Qt::AlignCenter);
}
