#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QSoundEffect>

class AudioManager : public QObject {
    Q_OBJECT

public:
    explicit AudioManager(QObject* parent = nullptr);

    bool isMuted() const { return m_muted; }
    void setMuted(bool muted) { m_muted = muted; }
    bool toggleMute();

    void setVolume(float volume);
    float getVolume() const { return m_volume; }

    void playMove();
    void playRotate();
    void playDrop();
    void playLock();
    void playClear(int combo = 1);
    void playGameOver();

private:
    void initAudioAssets();
    void loadEffect(const QString& name, const QString& filepath);
    void playEffect(const QString& name);

    bool m_muted;
    float m_volume;
    QMap<QString, QSoundEffect*> m_effects;
};
