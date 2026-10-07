#include <QApplication>
#include <QPalette>
#include <QColor>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    // Global Dark Palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor("#0b0d13"));
    darkPalette.setColor(QPalette::WindowText, QColor("#e2e8f0"));
    darkPalette.setColor(QPalette::Base, QColor("#161822"));
    darkPalette.setColor(QPalette::AlternateBase, QColor("#1a1e2d"));
    darkPalette.setColor(QPalette::ToolTipBase, QColor("#e2e8f0"));
    darkPalette.setColor(QPalette::ToolTipText, QColor("#0b0d13"));
    darkPalette.setColor(QPalette::Text, QColor("#e2e8f0"));
    darkPalette.setColor(QPalette::Button, QColor("#1a1e2d"));
    darkPalette.setColor(QPalette::ButtonText, QColor("#e2e8f0"));
    darkPalette.setColor(QPalette::Highlight, QColor("#00E6FF"));
    darkPalette.setColor(QPalette::HighlightedText, QColor("#0b0d13"));
    app.setPalette(darkPalette);

    MainWindow window;
    window.show();

    return app.exec();
}
