#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <vector>
#include "Config.h"
#include "AudioManager.h"
#include "GameCanvas.h"
#include "UIPanels.h"
#include "MenuWidgets.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void syncUI();
    void onStartGame();
    void onOpenSettings();
    void onOpenHowToPlay();
    void onReturnToMenu();
    void onSettingsChanged(const GameSettings& settings);
    void onToggleSound();
    void onRestartGame();

private:
    void initStackedViews();
    QWidget* createGameScreen();

    GameSettings m_settings;
    AudioManager m_audio;

    QStackedWidget* m_stack;
    MainMenuWidget* m_mainMenu;
    SettingsWidget* m_settingsView;
    HowToPlayWidget* m_howToPlayView;
    QWidget* m_gameScreen;

    // In-game HUD widgets
    GameCanvas* m_canvas;
    PiecePreviewWidget* m_holdPreview;
    StatCard* m_cardScore;
    StatCard* m_cardHighScore;
    StatCard* m_cardLevel;
    StatCard* m_cardBands;
    ComboBadge* m_comboBadge;

    std::vector<PiecePreviewWidget*> m_nextPreviews;
    QLabel* m_lblLockedElements;
    QPushButton* m_btnSound;
    QPushButton* m_btnRestart;
    QPushButton* m_btnMenu;
    ControlsGuideWidget* m_controlsGuide;
};
