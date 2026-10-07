#pragma once

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <vector>
#include "Config.h"
#include "AudioManager.h"
#include "GameCanvas.h"
#include "UIPanels.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void syncUI();
    void onPaletteChanged(int index);
    void onToggleSound();
    void onRestartGame();

private:
    void initLayout();

    AudioManager m_audio;
    GameCanvas* m_canvas;

    PiecePreviewWidget* m_holdPreview;
    StatCard* m_cardScore;
    StatCard* m_cardHighScore;
    StatCard* m_cardLevel;
    StatCard* m_cardBands;
    ComboBadge* m_comboBadge;

    std::vector<PiecePreviewWidget*> m_nextPreviews;
    QComboBox* m_comboPalette;
    QPushButton* m_btnSound;
    QPushButton* m_btnRestart;
    ControlsGuideWidget* m_controlsGuide;
};
