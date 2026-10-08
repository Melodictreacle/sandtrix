#include "MenuWidgets.h"
#include <QFrame>
#include <QScrollArea>
#include <QCoreApplication>
#include <QFont>
#include <QRadioButton>
#include <algorithm>

// ==================== MainMenuWidget ====================

MainMenuWidget::MainMenuWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(40, 35, 40, 35);
    mainLayout->setSpacing(16);

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
    mainLayout->addSpacing(10);

    // Menu Buttons Frame
    auto* menuCard = new QFrame(this);
    menuCard->setObjectName("menuCard");
    menuCard->setFixedWidth(340);
    menuCard->setStyleSheet(
        "QFrame#menuCard {"
        "  background-color: #161b26;"
        "  border: 1px solid #283248;"
        "  border-radius: 14px;"
        "  padding: 18px;"
        "}"
    );

    auto* menuLayout = new QVBoxLayout(menuCard);
    menuLayout->setSpacing(12);

    // Quick Mode Switcher Button on the Main Menu!
    m_btnModeToggle = new QPushButton(this);
    m_btnModeToggle->setFixedHeight(44);
    m_btnModeToggle->setCursor(Qt::PointingHandCursor);
    m_btnModeToggle->setToolTip("Click to cycle between Sandtrix (Elements OFF), Sandtrix (Elements ON), and Classic Tetris");
    connect(m_btnModeToggle, &QPushButton::clicked, this, &MainMenuWidget::modeToggled);

    auto* btnStart = new QPushButton("START GAME", this);
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

    auto* btnSettings = new QPushButton("SETTINGS", this);
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

    auto* btnHowToPlay = new QPushButton("HOW TO PLAY", this);
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

    auto* btnExit = new QPushButton("EXIT GAME", this);
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

    menuLayout->addWidget(m_btnModeToggle);
    menuLayout->addWidget(btnStart);
    menuLayout->addWidget(btnSettings);
    menuLayout->addWidget(btnHowToPlay);
    menuLayout->addWidget(btnExit);

    mainLayout->addWidget(menuCard, 0, Qt::AlignCenter);
    mainLayout->addSpacing(10);
}

void MainMenuWidget::updateSettingsBadge(const GameSettings& settings) {
    QString modeBadge;
    if (settings.gameMode == GameMode::Mystery) {
        modeBadge = "Mystery Sandtrix";
    } else if (settings.gameMode == GameMode::MysteryClassic) {
        modeBadge = "Troll Classic Tetris";
    } else if (settings.gameMode == GameMode::ClassicTetris) {
        modeBadge = "Classic Tetris";
    } else {
        modeBadge = settings.enableElements ? "Sandtrix (Elements: ON)" : "Sandtrix (Elements: OFF)";
    }
    QString sndStr = settings.soundEnabled ? "Sound ON" : "Sound OFF";

    m_lblBadge->setText(QString("Level %1  •  %2  •  %3")
        .arg(settings.startingLevel).arg(modeBadge).arg(sndStr));

    if (m_btnModeToggle) {
        if (settings.gameMode == GameMode::Mystery) {
            m_btnModeToggle->setText("MODE: MYSTERY SANDTRIX (CHAOS)");
            m_btnModeToggle->setStyleSheet(
                "QPushButton {"
                "  background-color: #3b1728;"
                "  color: #f687b3;"
                "  font-size: 11px;"
                "  font-weight: 800;"
                "  border: 1.5px solid #97266d;"
                "  border-radius: 8px;"
                "  letter-spacing: 0.5px;"
                "}"
                "QPushButton:hover {"
                "  background-color: #521b37;"
                "  border-color: #d53f8c;"
                "  color: #fbb6ce;"
                "}"
                "QPushButton:pressed {"
                "  background-color: #2b0e1d;"
                "}"
            );
        } else if (settings.gameMode == GameMode::MysteryClassic) {
            m_btnModeToggle->setText("MODE: TROLL CLASSIC TETRIS");
            m_btnModeToggle->setStyleSheet(
                "QPushButton {"
                "  background-color: #3b2317;"
                "  color: #ed8936;"
                "  font-size: 11px;"
                "  font-weight: 800;"
                "  border: 1.5px solid #dd6b20;"
                "  border-radius: 8px;"
                "  letter-spacing: 0.5px;"
                "}"
                "QPushButton:hover {"
                "  background-color: #4d2d1d;"
                "  border-color: #ed8936;"
                "  color: #fbd38d;"
                "}"
                "QPushButton:pressed {"
                "  background-color: #2b180d;"
                "}"
            );
        } else if (settings.gameMode == GameMode::ClassicTetris) {
            m_btnModeToggle->setText("MODE: CLASSIC TETRIS");
            m_btnModeToggle->setStyleSheet(
                "QPushButton {"
                "  background-color: #261f38;"
                "  color: #b794f4;"
                "  font-size: 12px;"
                "  font-weight: 800;"
                "  border: 1.5px solid #553c9a;"
                "  border-radius: 8px;"
                "  letter-spacing: 0.5px;"
                "}"
                "QPushButton:hover {"
                "  background-color: #32264d;"
                "  border-color: #805ad5;"
                "  color: #d6bcfa;"
                "}"
                "QPushButton:pressed {"
                "  background-color: #1c152a;"
                "}"
            );
        } else if (settings.enableElements) {
            m_btnModeToggle->setText("MODE: SANDTRIX (ELEMENTS: ON)");
            m_btnModeToggle->setStyleSheet(
                "QPushButton {"
                "  background-color: #142834;"
                "  color: #00d2d7;"
                "  font-size: 12px;"
                "  font-weight: 800;"
                "  border: 1.5px solid #1f576e;"
                "  border-radius: 8px;"
                "  letter-spacing: 0.5px;"
                "}"
                "QPushButton:hover {"
                "  background-color: #1a3749;"
                "  border-color: #38b2ac;"
                "  color: #81e6d9;"
                "}"
                "QPushButton:pressed {"
                "  background-color: #0f1e27;"
                "}"
            );
        } else {
            m_btnModeToggle->setText("MODE: SANDTRIX (ELEMENTS: OFF)");
            m_btnModeToggle->setStyleSheet(
                "QPushButton {"
                "  background-color: #172b22;"
                "  color: #68c48a;"
                "  font-size: 12px;"
                "  font-weight: 800;"
                "  border: 1.5px solid #285e43;"
                "  border-radius: 8px;"
                "  letter-spacing: 0.5px;"
                "}"
                "QPushButton:hover {"
                "  background-color: #1f3b2e;"
                "  border-color: #38a169;"
                "  color: #a7f3d0;"
                "}"
                "QPushButton:pressed {"
                "  background-color: #12211a;"
                "}"
            );
        }
    }
}

// ==================== SettingsWidget ====================

SettingsWidget::SettingsWidget(const GameSettings& currentSettings, QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(40, 20, 40, 20);
    mainLayout->setSpacing(12);

    auto* header = new QLabel("GAME SETTINGS", this);
    header->setStyleSheet("color: #7cb7ea; font-size: 26px; font-weight: 900; letter-spacing: 2px;");
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    auto* notice = new QLabel("Configure your game mode, starting level speed, and sound preferences.", this);
    notice->setStyleSheet("color: #94a1b2; font-size: 11px;");
    notice->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(notice);

    auto* card = new QFrame(this);
    card->setObjectName("settingsCard");
    card->setFixedWidth(480);
    card->setStyleSheet(
        "QFrame#settingsCard {"
        "  background-color: #161b26;"
        "  border: 1px solid #283248;"
        "  border-radius: 14px;"
        "  padding: 16px;"
        "}"
    );
    auto* formLayout = new QVBoxLayout(card);
    formLayout->setSpacing(14);

    // 1. GAME MODE SELECTION
    auto* lblModeTitle = new QLabel("GAME MODE SELECTION", this);
    lblModeTitle->setStyleSheet("color: #94a1b2; font-size: 11px; font-weight: bold; letter-spacing: 1px;");
    formLayout->addWidget(lblModeTitle);

    auto* modeBox = new QFrame(this);
    modeBox->setObjectName("modeBox");
    modeBox->setStyleSheet(
        "QFrame#modeBox {"
        "  background-color: #10141e;"
        "  border: 1px solid #283248;"
        "  border-radius: 10px;"
        "  padding: 10px;"
        "}"
    );
    auto* modeLayout = new QVBoxLayout(modeBox);
    modeLayout->setSpacing(8);

    // Sandtrix Radio Button
    m_rbSandtrix = new QRadioButton("Sandtrix (Sand Physics Mode)", this);
    m_rbSandtrix->setCursor(Qt::PointingHandCursor);
    m_rbSandtrix->setStyleSheet(
        "QRadioButton {"
        "  color: #68c48a;"
        "  font-size: 13px;"
        "  font-weight: 800;"
        "}"
        "QRadioButton::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "}"
    );

    auto* lblSandtrixDesc = new QLabel("  • Mino blocks dissolve into loose granular sand upon contact\n  • Avalanche cascade physics & 8-way wall-to-wall flood fill clears", this);
    lblSandtrixDesc->setStyleSheet("color: #94a1b2; font-size: 10px; margin-left: 24px; line-height: 1.3;");

    // Optional elemental pieces checkbox for Sandtrix
    m_chkElements = new QCheckBox("Enable Elemental Pieces (Water & Acid)", this);
    m_chkElements->setCursor(Qt::PointingHandCursor);
    m_chkElements->setStyleSheet(
        "QCheckBox {"
        "  color: #00d2d7;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "  margin-left: 24px;"
        "}"
        "QCheckBox::indicator {"
        "  width: 16px;"
        "  height: 16px;"
        "}"
    );

    // Classic Tetris Radio Button
    m_rbClassicTetris = new QRadioButton("Classic Tetris (Solid Grid Mode)", this);
    m_rbClassicTetris->setCursor(Qt::PointingHandCursor);
    m_rbClassicTetris->setStyleSheet(
        "QRadioButton {"
        "  color: #b794f4;"
        "  font-size: 13px;"
        "  font-weight: 800;"
        "}"
        "QRadioButton::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "}"
    );

    auto* lblClassicDesc = new QLabel("  • Authentic traditional Tetris on a 10×17 mino grid (no sand physics)\n  • Solid rigid blocks: pieces stay locked without melting into sand\n  • Traditional full horizontal line clears (Single, Double, Triple, TETRIS!)", this);
    lblClassicDesc->setStyleSheet("color: #94a1b2; font-size: 10px; margin-left: 24px; line-height: 1.3;");

    // Mystery Mode Radio Button
    m_rbMystery = new QRadioButton("Mystery Sandtrix (Shape-Shifting & Sand Chaos)", this);
    m_rbMystery->setCursor(Qt::PointingHandCursor);
    m_rbMystery->setStyleSheet(
        "QRadioButton {"
        "  color: #f687b3;"
        "  font-size: 13px;"
        "  font-weight: 800;"
        "}"
        "QRadioButton::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "}"
    );

    auto* lblMysteryDesc = new QLabel("  • Mid-air shape-shifting & piece mutations: blocks transform while falling!\n  • Wildcard TNT bomb pieces, mini-dots, plus pieces, earthquakes & wind gusts!\n  • Unpredictable, wacky, and hilarious sand physics gameplay", this);
    lblMysteryDesc->setStyleSheet("color: #94a1b2; font-size: 10px; margin-left: 24px; line-height: 1.3;");

    // Troll Classic Tetris Radio Button
    m_rbMysteryClassic = new QRadioButton("Troll Classic Tetris (Chaos & Sarcastic Roasts)", this);
    m_rbMysteryClassic->setCursor(Qt::PointingHandCursor);
    m_rbMysteryClassic->setStyleSheet(
        "QRadioButton {"
        "  color: #ed8936;"
        "  font-size: 13px;"
        "  font-weight: 800;"
        "}"
        "QRadioButton::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "}"
    );

    auto* lblMysteryClassicDesc = new QLabel(
        "  • Authentic 10x17 rigid Tetris grid hijacked with mid-air piece mutations & shape shifts!\n"
        "  • TNT bomb blocks, mini-dots, plus pieces, earthquakes & wind gusts blowing pieces off-course!\n"
        "  • Sarcastic troll roasts on singles, doubles, near-death misplays, and hard drops!", this);
    lblMysteryClassicDesc->setStyleSheet("color: #94a1b2; font-size: 10px; margin-left: 24px; line-height: 1.3;");

    modeLayout->addWidget(m_rbSandtrix);
    modeLayout->addWidget(lblSandtrixDesc);
    modeLayout->addWidget(m_chkElements);
    modeLayout->addSpacing(4);
    modeLayout->addWidget(m_rbClassicTetris);
    modeLayout->addWidget(lblClassicDesc);
    modeLayout->addSpacing(4);
    modeLayout->addWidget(m_rbMystery);
    modeLayout->addWidget(lblMysteryDesc);
    modeLayout->addSpacing(4);
    modeLayout->addWidget(m_rbMysteryClassic);
    modeLayout->addWidget(lblMysteryClassicDesc);
    formLayout->addWidget(modeBox);

    connect(m_rbSandtrix, &QRadioButton::toggled, this, &SettingsWidget::onModeChanged);
    connect(m_rbClassicTetris, &QRadioButton::toggled, this, &SettingsWidget::onModeChanged);
    connect(m_rbMystery, &QRadioButton::toggled, this, &SettingsWidget::onModeChanged);
    connect(m_rbMysteryClassic, &QRadioButton::toggled, this, &SettingsWidget::onModeChanged);
    connect(m_chkElements, &QCheckBox::toggled, this, &SettingsWidget::updateModeLabels);

    // 2. STARTING SPEED / LEVEL (1 - 10)
    auto* lblLevelTitle = new QLabel("STARTING SPEED / LEVEL (1 - 10)", this);
    lblLevelTitle->setStyleSheet("color: #94a1b2; font-size: 11px; font-weight: bold; letter-spacing: 1px;");
    formLayout->addWidget(lblLevelTitle);

    auto* levelBox = new QFrame(this);
    levelBox->setObjectName("levelBox");
    levelBox->setStyleSheet(
        "QFrame#levelBox {"
        "  background-color: #10141e;"
        "  border: 1px solid #283248;"
        "  border-radius: 10px;"
        "  padding: 10px;"
        "}"
    );
    auto* levelBoxLayout = new QVBoxLayout(levelBox);
    levelBoxLayout->setSpacing(8);

    auto* levelRow = new QHBoxLayout();
    levelRow->setSpacing(10);

    m_btnLevelDown = new QPushButton("◀  −", this);
    m_btnLevelDown->setFixedSize(70, 42);
    m_btnLevelDown->setCursor(Qt::PointingHandCursor);
    m_btnLevelDown->setToolTip("Decrease Starting Level");
    m_btnLevelDown->setStyleSheet(
        "QPushButton {"
        "  background-color: #242c3d;"
        "  color: #f7fafc;"
        "  border: 1px solid #37435f;"
        "  border-radius: 8px;"
        "  font-size: 15px;"
        "  font-weight: 900;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2f3a50;"
        "  border-color: #68c48a;"
        "  color: #68c48a;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1a202c;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #181d27;"
        "  color: #4a5568;"
        "  border-color: #242b38;"
        "}"
    );

    m_spinLevel = new QSpinBox(this);
    m_spinLevel->setRange(1, 10);
    m_spinLevel->setValue(currentSettings.startingLevel);
    m_spinLevel->setAlignment(Qt::AlignCenter);
    m_spinLevel->setFixedHeight(42);
    m_spinLevel->setPrefix("Level  ");
    m_spinLevel->setStyleSheet(
        "QSpinBox {"
        "  background-color: #1a202c;"
        "  border: 1px solid #37435f;"
        "  border-radius: 8px;"
        "  padding: 4px 10px;"
        "  color: #68c48a;"
        "  font-size: 16px;"
        "  font-weight: 800;"
        "}"
        "QSpinBox:hover {"
        "  border-color: #68c48a;"
        "}"
        "QSpinBox::up-button {"
        "  subcontrol-origin: border;"
        "  subcontrol-position: top right;"
        "  width: 28px;"
        "  background-color: #242c3d;"
        "  border-left: 1px solid #37435f;"
        "  border-bottom: 1px solid #37435f;"
        "  border-top-right-radius: 7px;"
        "}"
        "QSpinBox::up-button:hover {"
        "  background-color: #313c54;"
        "}"
        "QSpinBox::down-button {"
        "  subcontrol-origin: border;"
        "  subcontrol-position: bottom right;"
        "  width: 28px;"
        "  background-color: #242c3d;"
        "  border-left: 1px solid #37435f;"
        "  border-bottom-right-radius: 7px;"
        "}"
        "QSpinBox::down-button:hover {"
        "  background-color: #313c54;"
        "}"
        "QSpinBox::up-arrow {"
        "  width: 7px;"
        "  height: 7px;"
        "}"
        "QSpinBox::down-arrow {"
        "  width: 7px;"
        "  height: 7px;"
        "}"
    );

    m_btnLevelUp = new QPushButton("+  ▶", this);
    m_btnLevelUp->setFixedSize(70, 42);
    m_btnLevelUp->setCursor(Qt::PointingHandCursor);
    m_btnLevelUp->setToolTip("Increase Starting Level");
    m_btnLevelUp->setStyleSheet(
        "QPushButton {"
        "  background-color: #242c3d;"
        "  color: #f7fafc;"
        "  border: 1px solid #37435f;"
        "  border-radius: 8px;"
        "  font-size: 15px;"
        "  font-weight: 900;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2f3a50;"
        "  border-color: #68c48a;"
        "  color: #68c48a;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1a202c;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #181d27;"
        "  color: #4a5568;"
        "  border-color: #242b38;"
        "}"
    );

    levelRow->addWidget(m_btnLevelDown);
    levelRow->addWidget(m_spinLevel, 1);
    levelRow->addWidget(m_btnLevelUp);
    levelBoxLayout->addLayout(levelRow);

    m_lblLevelDesc = new QLabel(this);
    m_lblLevelDesc->setAlignment(Qt::AlignCenter);
    m_lblLevelDesc->setStyleSheet("color: #a0aec0; font-size: 11px; font-weight: 600;");
    levelBoxLayout->addWidget(m_lblLevelDesc);

    formLayout->addWidget(levelBox);

    connect(m_btnLevelDown, &QPushButton::clicked, this, &SettingsWidget::onLevelDown);
    connect(m_btnLevelUp, &QPushButton::clicked, this, &SettingsWidget::onLevelUp);
    connect(m_spinLevel, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsWidget::updateLevelDesc);

    // 3. DISPLAY & AUDIO
    auto* lblAudioTitle = new QLabel("DISPLAY & AUDIO", this);
    lblAudioTitle->setStyleSheet("color: #94a1b2; font-size: 11px; font-weight: bold; letter-spacing: 1px;");
    formLayout->addWidget(lblAudioTitle);

    auto* audioBox = new QFrame(this);
    audioBox->setObjectName("audioBox");
    audioBox->setStyleSheet(
        "QFrame#audioBox {"
        "  background-color: #10141e;"
        "  border: 1px solid #283248;"
        "  border-radius: 10px;"
        "  padding: 10px;"
        "}"
    );
    auto* audioLayout = new QVBoxLayout(audioBox);
    audioLayout->setSpacing(10);

    m_chkFullscreen = new QCheckBox("Enable Fullscreen Display (or press F11 anytime)", this);
    m_chkFullscreen->setCursor(Qt::PointingHandCursor);
    m_chkFullscreen->setStyleSheet(
        "QCheckBox {"
        "  color: #a78bfa;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "}"
        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "}"
    );

    m_chkSound = new QCheckBox("Enable Procedural Sound Effects", this);
    m_chkSound->setCursor(Qt::PointingHandCursor);
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

    auto* volRow = new QHBoxLayout();
    volRow->setSpacing(10);

    m_lblSpeakerIcon = new QLabel("VOL", this);
    m_lblSpeakerIcon->setStyleSheet("font-size: 11px; font-weight: bold; color: #7cb7ea; background: transparent; border: none; padding: 0px;");

    m_sliderVolume = new QSlider(Qt::Horizontal, this);
    m_sliderVolume->setRange(0, 100);
    m_sliderVolume->setValue(static_cast<int>(currentSettings.volume * 100.0f));
    m_sliderVolume->setToolTip("Adjust sound effects volume (0% - 100%)");
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

    m_lblVolume = new QLabel(QString("%1%").arg(m_sliderVolume->value()), this);
    m_lblVolume->setFixedWidth(60);
    m_lblVolume->setAlignment(Qt::AlignCenter);
    m_lblVolume->setToolTip("Volume level percentage");

    volRow->addWidget(m_lblSpeakerIcon);
    volRow->addWidget(m_sliderVolume, 1);
    volRow->addWidget(m_lblVolume);

    audioLayout->addWidget(m_chkFullscreen);
    audioLayout->addWidget(m_chkSound);
    audioLayout->addLayout(volRow);
    formLayout->addWidget(audioBox);

    connect(m_chkSound, &QCheckBox::toggled, this, &SettingsWidget::updateSoundWidgets);
    connect(m_sliderVolume, &QSlider::valueChanged, this, &SettingsWidget::updateSoundWidgets);

    mainLayout->addWidget(card, 0, Qt::AlignCenter);

    // Save & Cancel buttons
    auto* btnRowLayout = new QHBoxLayout();
    btnRowLayout->setSpacing(12);

    auto* btnSave = new QPushButton("SAVE & APPLY", this);
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

    auto* btnCancel = new QPushButton("BACK", this);
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

    loadSettings(currentSettings);
}

void SettingsWidget::updateModeLabels() {
    bool elemOn = m_chkElements->isChecked();
    if (elemOn) {
        m_rbSandtrix->setText("Sandtrix (Sand Physics Mode)  •  [Elements: ON]");
        m_chkElements->setText("Enable Elemental Pieces (Water & Acid) — Currently ON");
    } else {
        m_rbSandtrix->setText("Sandtrix (Sand Physics Mode)  •  [Elements: OFF]");
        m_chkElements->setText("Enable Elemental Pieces (Water & Acid) — Currently OFF");
    }
}

void SettingsWidget::onModeChanged() {
    bool isSandtrix = m_rbSandtrix->isChecked();
    m_chkElements->setEnabled(isSandtrix);
    updateModeLabels();
}

void SettingsWidget::updateSoundWidgets() {
    bool enabled = m_chkSound->isChecked();
    m_sliderVolume->setEnabled(enabled);
    if (m_lblSpeakerIcon) {
        m_lblSpeakerIcon->setText(enabled ? "VOL" : "MUTE");
        m_lblSpeakerIcon->setStyleSheet(enabled ? "font-size: 11px; font-weight: bold; color: #7cb7ea; background: transparent; border: none; padding: 0px;" : "font-size: 11px; font-weight: bold; color: #718096; background: transparent; border: none; padding: 0px;");
    }
    if (enabled) {
        m_lblVolume->setText(QString("%1%").arg(m_sliderVolume->value()));
        m_lblVolume->setStyleSheet(
            "QLabel {"
            "  background-color: #1a2332;"
            "  border: 1px solid #285e43;"
            "  border-radius: 6px;"
            "  color: #68c48a;"
            "  font-weight: bold;"
            "  font-size: 12px;"
            "  padding: 4px 6px;"
            "}"
        );
    } else {
        m_lblVolume->setText("MUTED");
        m_lblVolume->setStyleSheet(
            "QLabel {"
            "  background-color: #181c24;"
            "  border: 1px solid #37435f;"
            "  border-radius: 6px;"
            "  color: #718096;"
            "  font-weight: bold;"
            "  font-size: 11px;"
            "  padding: 4px 6px;"
            "}"
        );
    }
}

void SettingsWidget::loadSettings(const GameSettings& settings) {
    m_spinLevel->setValue(settings.startingLevel);
    m_rbSandtrix->setChecked(settings.gameMode == GameMode::Sandtrix);
    m_rbClassicTetris->setChecked(settings.gameMode == GameMode::ClassicTetris);
    m_rbMystery->setChecked(settings.gameMode == GameMode::Mystery);
    m_rbMysteryClassic->setChecked(settings.gameMode == GameMode::MysteryClassic);
    m_chkElements->setChecked(settings.enableElements);
    m_chkElements->setEnabled(settings.gameMode == GameMode::Sandtrix);
    m_chkFullscreen->setChecked(settings.fullscreen);
    m_chkSound->setChecked(settings.soundEnabled);
    m_sliderVolume->setValue(static_cast<int>(settings.volume * 100.0f));
    updateSoundWidgets();
    updateLevelDesc();
    updateModeLabels();
}

GameSettings SettingsWidget::getSettings() const {
    GameSettings s;
    s.paletteIndex = 0;
    s.startingLevel = m_spinLevel->value();
    if (m_rbClassicTetris->isChecked()) {
        s.gameMode = GameMode::ClassicTetris;
    } else if (m_rbMystery->isChecked()) {
        s.gameMode = GameMode::Mystery;
    } else if (m_rbMysteryClassic->isChecked()) {
        s.gameMode = GameMode::MysteryClassic;
    } else {
        s.gameMode = GameMode::Sandtrix;
    }
    s.enableElements = (s.gameMode == GameMode::Sandtrix) && m_chkElements->isChecked();
    s.fullscreen = m_chkFullscreen->isChecked();
    s.soundEnabled = m_chkSound->isChecked();
    s.volume = m_sliderVolume->value() / 100.0f;
    return s;
}

void SettingsWidget::onSaveClicked() {
    emit settingsChanged(getSettings());
    emit backClicked();
}

void SettingsWidget::onLevelDown() {
    int val = m_spinLevel->value();
    if (val > 1) {
        m_spinLevel->setValue(val - 1);
    }
}

void SettingsWidget::onLevelUp() {
    int val = m_spinLevel->value();
    if (val < 10) {
        m_spinLevel->setValue(val + 1);
    }
}

void SettingsWidget::updateLevelDesc() {
    int val = m_spinLevel->value();
    m_btnLevelDown->setEnabled(val > 1);
    m_btnLevelUp->setEnabled(val < 10);

    static const char* descs[] = {
        "Level 1 • Relaxed Pace (45ms drop delay)",
        "Level 2 • Gentle Pace (40ms drop delay)",
        "Level 3 • Standard Pace (35ms drop delay)",
        "Level 4 • Moderate Pace (30ms drop delay)",
        "Level 5 • Brisk Pace (25ms drop delay)",
        "Level 6 • Fast Pace (20ms drop delay)",
        "Level 7 • Rapid Pace (16ms drop delay)",
        "Level 8 • Very Fast Pace (13ms drop delay)",
        "Level 9 • Extreme Pace (10ms drop delay)",
        "Level 10 • Maximum Speed (8ms drop delay)"
    };
    int idx = std::max(1, std::min(10, val)) - 1;
    m_lblLevelDesc->setText(descs[idx]);
}

// ==================== HowToPlayWidget ====================

HowToPlayWidget::HowToPlayWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(40, 20, 40, 20);
    mainLayout->setSpacing(14);

    auto* header = new QLabel("HOW TO PLAY SANDTRIX", this);
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

    addCard("1. The Sand Dissolution Rule",
            "Blocks fall as classic Tetris tetrominoes (I, O, T, S, Z, J, L) that you can rotate, steer, and hold. "
            "However, the instant a block hits the ground or resting sand, it dissolves into hundreds of loose sand grains!",
            "#7cb7ea");

    addCard("2. Edge-to-Edge Line Clears",
            "Unlike traditional Tetris where straight horizontal rows clear, in Sandtrix you clear lines by forming an "
            "unbroken path of the SAME COLOR (Red, Gold, Green, or Blue) connecting the LEFT WALL (x=0) to the RIGHT WALL (x=W-1). "
            "Any 8-way continuous connected path of that color vaporizes!",
            "#e2b755");

    addCard("3. Avalanche Combos",
            "When a colored band clears, all sand resting on top avalanches down into the gap. "
            "If the collapsing cascade creates a NEW connection between the walls, a COMBO CLEAR triggers with exponential score multipliers!",
            "#e27d9a");

    addCard("4. Elemental Pieces (Sandtrix Mode)",
            "• Water Blocks (Aqua): Liquid flows horizontally to fill gaps flat. Heavy sand sinks through water! Connecting water edge-to-edge triggers a Tidal Wave (+800 pts)!\n"
            "• Acid Blocks (Lime): Distinct radioactive lime blocks that actively dissolve sand directly beneath them on impact.\n"
            "• Note: Elemental pieces can be enabled or disabled in Settings for Sandtrix mode.",
            "#68c48a");

    addCard("5. Classic Tetris Mode",
            "• Switch to 'Classic Tetris' on the Main Menu or in Settings to play authentic traditional grid Tetris!\n"
            "• Rigid solid blocks on a 10×17 mino grid: pieces do NOT dissolve into sand.\n"
            "• Complete full horizontal rows to score Single, Double, Triple, and 4-line TETRIS clears!",
            "#b794f4");

    addCard("6. Mystery Sandtrix (Shape-Shifting & Chaos)",
            "• Mid-Air Shape-Shifting: Falling pieces spontaneously mutate mid-flight into different tetrominoes, tiny 1x1 dots, or 5-block plus crosses!\n"
            "• TNT Bomb Blocks: Flashing explosive blocks that blast an impact crater through resting sand heaps.\n"
            "• Wacky Calamities: Earthquakes that shift the dunes, sudden gusts of wind, and temporary inverted controls!",
            "#f687b3");

    addCard("7. Troll Classic Tetris (Grid Chaos & Sarcastic Roasts)",
            "• Authentic traditional 10x17 rigid grid Tetris meets unpredictable chaos!\n"
            "• Mid-air mutations: blocks unexpectedly transform into other shapes or wildcard 1x1 dots and plus signs!\n"
            "• Calamities: explosive TNT blocks blasting craters in your stack, inverted controls, and sudden wind gusts!\n"
            "• Sarcastic commentary: the game actively roasts single and double clears, near-death moments, and misplays!",
            "#ed8936");

    addCard("8. Complete Keyboard Controls",
            "• [Left / Right] or [A / D] : Move Left / Right (DAS & ARR enabled)\n"
            "• [Up] or [W / X] : Rotate Clockwise\n"
            "• [Z / Ctrl] : Rotate Counter-Clockwise\n"
            "• [Down / S] : Soft Drop\n"
            "• [SPACE] : Hard Drop\n"
            "• [C / Shift] : Hold Piece\n"
            "• [P / Esc] : Pause / Resume\n"
            "• [R] : Restart Run\n"
            "• [M] : Mute Sound",
            "#b89bf0");

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea, 0, Qt::AlignCenter);

    auto* btnBack = new QPushButton("BACK TO MENU", this);
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
