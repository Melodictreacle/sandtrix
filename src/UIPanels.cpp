#include "UIPanels.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QFont>
#include <algorithm>

StatCard::StatCard(const QString& title, const QString& initialValue, const QString& accentColor, QWidget* parent)
    : QFrame(parent)
{
    setStyleSheet(
        "QFrame {"
        "  background-color: #161b26;"
        "  border: 1px solid #283248;"
        "  border-radius: 8px;"
        "  padding: 6px;"
        "}"
    );

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(2);

    m_lblTitle = new QLabel(title.toUpper(), this);
    m_lblTitle->setStyleSheet("color: #94a1b2; font-size: 11px; font-weight: bold; letter-spacing: 1px;");

    m_lblVal = new QLabel(initialValue, this);
    m_lblVal->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: bold; font-family: 'Segoe UI', sans-serif;").arg(accentColor));

    layout->addWidget(m_lblTitle);
    layout->addWidget(m_lblVal);
}

void StatCard::setValue(const QString& value) {
    m_lblVal->setText(value);
}

// ----------------- ComboBadge -----------------

ComboBadge::ComboBadge(QWidget* parent)
    : QFrame(parent)
{
    setStyleSheet(
        "QFrame {"
        "  background-color: #261a22;"
        "  border: 1px solid #78364b;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "}"
    );

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 6, 10, 6);

    m_lblCombo = new QLabel("COMBO: --", this);
    m_lblCombo->setAlignment(Qt::AlignCenter);
    m_lblCombo->setStyleSheet("color: #e27d9a; font-size: 14px; font-weight: bold; letter-spacing: 1px;");
    layout->addWidget(m_lblCombo);

    setVisible(false);
}

void ComboBadge::updateCombo(int combo) {
    if (combo > 1) {
        m_lblCombo->setText(QString("COMBO x%1!").arg(combo));
        setVisible(true);
    } else {
        setVisible(false);
    }
}

// ----------------- PiecePreviewWidget -----------------

PiecePreviewWidget::PiecePreviewWidget(const QString& title, const ColorPalette& palette, int boxSize, QWidget* parent)
    : QWidget(parent), m_title(title), m_palette(palette), m_boxSize(boxSize)
{
    setFixedSize(boxSize, boxSize + 24);
}

void PiecePreviewWidget::setPalette(const ColorPalette& palette) {
    m_palette = palette;
    update();
}

void PiecePreviewWidget::setPiece(const Tetromino* piece) {
    if (piece) {
        m_cachedPiece = std::make_unique<Tetromino>(piece->getShape(), piece->getColorIdx());
    } else {
        m_cachedPiece.reset();
    }
    update();
}

void PiecePreviewWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Determine Title and Header Color
    QString displayTitle = m_title.toUpper();
    QColor titleCol("#7d8597");

    if (m_cachedPiece) {
        if (m_cachedPiece->isWater()) {
            displayTitle = "WATER";
            titleCol = QColor("#00E6FF");
        } else if (m_cachedPiece->isAcid()) {
            displayTitle = "ACID";
            titleCol = QColor("#39FF14");
        }
    }

    painter.setFont(QFont("Segoe UI", 8, QFont::Bold));
    painter.setPen(titleCol);
    painter.drawText(0, 14, displayTitle);

    // Draw Background Box
    int boxY = 20;
    painter.setPen(QPen(QColor("#2a2e3f"), 1));
    painter.setBrush(QBrush(QColor("#161822")));
    painter.drawRoundedRect(0, boxY, m_boxSize, m_boxSize, 6, 6);

    if (!m_cachedPiece) return;

    // Render Tetromino grains
    QColor grainColor;
    if (m_cachedPiece->isWater()) {
        grainColor = getWaterColor();
    } else if (m_cachedPiece->isAcid()) {
        grainColor = getAcidColor();
    } else {
        int colIdx = m_cachedPiece->getColorIdx();
        grainColor = m_palette.colors[colIdx % m_palette.colors.size()];
    }

    int rows = m_cachedPiece->getMaskHeight();
    int cols = m_cachedPiece->getMaskWidth();
    const auto& mask = m_cachedPiece->getGrainMask();

    double scale = std::min(
        (m_boxSize - 16.0) / cols,
        (m_boxSize - 16.0) / rows
    );

    double offsetX = (m_boxSize - cols * scale) / 2.0;
    double offsetY = boxY + (m_boxSize - rows * scale) / 2.0;

    painter.setPen(Qt::NoPen);
    painter.setBrush(grainColor);

    int pixelSize = std::max(1, static_cast<int>(scale));

    for (int gy = 0; gy < rows; ++gy) {
        for (int gx = 0; gx < cols; ++gx) {
            if (mask[gy * cols + gx]) {
                int px = static_cast<int>(offsetX + gx * scale);
                int py = static_cast<int>(offsetY + gy * scale);
                painter.drawRect(px, py, pixelSize, pixelSize);
            }
        }
    }
}

// ----------------- ControlsGuideWidget -----------------

ControlsGuideWidget::ControlsGuideWidget(QWidget* parent)
    : QFrame(parent)
{
    setStyleSheet(
        "QFrame {"
        "  background-color: #161b26;"
        "  border: 1px solid #283248;"
        "  border-radius: 8px;"
        "}"
    );

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(4);

    auto* header = new QLabel("CONTROLS", this);
    header->setStyleSheet("color: #94a1b2; font-size: 11px; font-weight: bold; letter-spacing: 1px;");
    layout->addWidget(header);

    const std::vector<std::pair<QString, QString>> shortcuts = {
        {"← / → / A / D", "Move"},
        {"↑ / W / X", "Rotate CW"},
        {"Z", "Rotate CCW"},
        {"↓ / S", "Soft Drop"},
        {"SPACE", "Hard Drop"},
        {"C / Shift", "Hold Piece"},
        {"F11", "Fullscreen"},
        {"P / Esc", "Pause"},
        {"R", "Restart"},
        {"M", "Mute Audio"}
    };

    for (const auto& [keys, desc] : shortcuts) {
        auto* row = new QHBoxLayout();
        row->setSpacing(4);

        auto* lblKey = new QLabel(keys, this);
        lblKey->setStyleSheet(
            "background-color: #242c3d;"
            "color: #7cb7ea;"
            "border: 1px solid #37435f;"
            "font-size: 10px;"
            "font-family: Consolas, monospace;"
            "padding: 2px 5px;"
            "border-radius: 4px;"
        );

        auto* lblDesc = new QLabel(desc, this);
        lblDesc->setStyleSheet("color: #cbd5e0; font-size: 10px;");

        row->addWidget(lblKey);
        row->addStretch();
        row->addWidget(lblDesc);
        layout->addLayout(row);
    }
}
