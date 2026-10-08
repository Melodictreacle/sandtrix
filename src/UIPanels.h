#pragma once

#include <QWidget>
#include <QFrame>
#include <QLabel>
#include "Config.h"
#include "Tetromino.h"

class StatCard : public QFrame {
    Q_OBJECT

public:
    StatCard(const QString& title, const QString& initialValue = "0", const QString& accentColor = "#00E6FF", QWidget* parent = nullptr);
    void setValue(const QString& value);
    void setTitle(const QString& title);

private:
    QLabel* m_lblTitle;
    QLabel* m_lblVal;
};

class ComboBadge : public QFrame {
    Q_OBJECT

public:
    explicit ComboBadge(QWidget* parent = nullptr);
    void updateCombo(int combo);

private:
    QLabel* m_lblCombo;
};

class PiecePreviewWidget : public QWidget {
    Q_OBJECT

public:
    PiecePreviewWidget(const QString& title, const ColorPalette& palette, int boxSize = 80, QWidget* parent = nullptr);

    void setPalette(const ColorPalette& palette);
    void setPiece(const Tetromino* piece);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_title;
    ColorPalette m_palette;
    int m_boxSize;
    std::unique_ptr<Tetromino> m_cachedPiece;
};

class ControlsGuideWidget : public QFrame {
    Q_OBJECT

public:
    explicit ControlsGuideWidget(QWidget* parent = nullptr);
};
