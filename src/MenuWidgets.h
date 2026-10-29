#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "Config.h"

class MainMenuWidget : public QWidget {
    Q_OBJECT

public:
    explicit MainMenuWidget(QWidget* parent = nullptr);
    void updateSettingsBadge(const GameSettings& settings);

signals:
    void startClicked();
    void settingsClicked();
    void howToPlayClicked();
    void exitClicked();
    void modeToggled();
    void levelChanged(int level);
    void goalChanged(int lineGoal);

private slots:
    void onLevelDown();
    void onLevelUp();
    void onLevelSpinChanged(int val);
    void onGoalPrev();
    void onGoalNext();
    void onGoalCycle();

private:
    void updateLevelDisplay();
    void updateGoalDisplay();
    static int getGoalValueFromIndex(int idx);
    static int getIndexFromGoalValue(int goal);

    QLabel* m_lblBadge;
    QPushButton* m_btnModeToggle;

    // Main Menu Level / Difficulty Stepper
    QSpinBox* m_spinLevel;
    QPushButton* m_btnLevelDown;
    QPushButton* m_btnLevelUp;
    QLabel* m_lblLevelDesc;

    // Main Menu Mission / Line Goal Stepper
    int m_currentGoalIndex = 0;
    QPushButton* m_btnGoalPrev;
    QPushButton* m_btnGoalNext;
    QPushButton* m_btnGoalValue;
    QLabel* m_lblGoalDesc;
};

class SettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWidget(const GameSettings& currentSettings, QWidget* parent = nullptr);
    GameSettings getSettings() const;
    void loadSettings(const GameSettings& settings);

signals:
    void backClicked();
    void settingsChanged(const GameSettings& settings);

private slots:
    void onSaveClicked();
    void onLevelDown();
    void onLevelUp();
    void updateLevelDesc();
    void onModeChanged();
    void updateGoalDesc();

private:
    void updateModeLabels();
    void updateSoundWidgets();

    // Level Stepper
    QSpinBox* m_spinLevel;
    QPushButton* m_btnLevelDown;
    QPushButton* m_btnLevelUp;
    QLabel* m_lblLevelDesc;

    // Game Mode Selection
    QRadioButton* m_rbSandtrix;
    QRadioButton* m_rbClassicTetris;
    QRadioButton* m_rbMystery;
    QRadioButton* m_rbMysteryClassic;
    QCheckBox* m_chkElements;

    // Mission / Line Goal Selection
    QRadioButton* m_rbGoalEndless;
    QRadioButton* m_rbGoal20;
    QRadioButton* m_rbGoal40;
    QRadioButton* m_rbGoal100;
    QLabel* m_lblGoalDesc;

    // Display & Audio
    QCheckBox* m_chkFullscreen;
    QCheckBox* m_chkSound;
    QLabel* m_lblSpeakerIcon;
    QSlider* m_sliderVolume;
    QLabel* m_lblVolume;
};

class HowToPlayWidget : public QWidget {
    Q_OBJECT

public:
    explicit HowToPlayWidget(QWidget* parent = nullptr);

signals:
    void backClicked();
};
