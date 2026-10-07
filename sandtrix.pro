QT += core gui widgets multimedia

CONFIG += c++17

TARGET = sandtrix
TEMPLATE = app

INCLUDEPATH += src

SOURCES += \
    src/main.cpp \
    src/Tetromino.cpp \
    src/SandEngine.cpp \
    src/ParticleSystem.cpp \
    src/AudioManager.cpp \
    src/UIPanels.cpp \
    src/GameCanvas.cpp \
    src/MainWindow.cpp \
    src/MenuWidgets.cpp

HEADERS += \
    src/Config.h \
    src/Tetromino.h \
    src/SandEngine.h \
    src/ParticleSystem.h \
    src/AudioManager.h \
    src/UIPanels.h \
    src/GameCanvas.h \
    src/MainWindow.h \
    src/MenuWidgets.h
