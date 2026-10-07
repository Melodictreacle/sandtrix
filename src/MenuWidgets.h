#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
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

private:
    QLabel* m_lblBadge;
};

class SettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWidget(const GameSettings& currentSettings, QWidget* parent = nullptr);
    GameSettings getSettings() const;

signals:
    void backClicked();
    void settingsChanged(const GameSettings& settings);

private slots:
    void onPaletteIndexChanged(int index);
    void onSaveClicked();

private:
    QComboBox* m_comboPalette;
    QLabel* m_lblPaletteDesc;
    QSpinBox* m_spinLevel;
    QCheckBox* m_chkElements;
    QCheckBox* m_chkSound;
    QSlider* m_sliderVolume;
};

class HowToPlayWidget : public QWidget {
    Q_OBJECT

public:
    explicit HowToPlayWidget(QWidget* parent = nullptr);

signals:
    void backClicked();
};
