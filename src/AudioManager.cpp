#include "AudioManager.h"
#include <QDir>
#include <QFile>
#include <QUrl>
#include <QFileInfo>
#include <cmath>
#include <vector>
#include <fstream>
#include <random>

namespace {
constexpr double PI = 3.14159265358979323846;

void writeWavHeader(std::ofstream& out, int totalSamples, int sampleRate = 44100) {
    int byteRate = sampleRate * 1 * 2;
    int dataChunkSize = totalSamples * 2;
    int fileSizeMinus8 = 36 + dataChunkSize;

    out.write("RIFF", 4);
    out.write(reinterpret_cast<const char*>(&fileSizeMinus8), 4);
    out.write("WAVE", 4);

    out.write("fmt ", 4);
    int subchunk1Size = 16;
    short audioFormat = 1; // PCM
    short numChannels = 1; // Mono
    int sRate = sampleRate;
    short blockAlign = 2;
    short bitsPerSample = 16;
    out.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
    out.write(reinterpret_cast<const char*>(&audioFormat), 2);
    out.write(reinterpret_cast<const char*>(&numChannels), 2);
    out.write(reinterpret_cast<const char*>(&sRate), 4);
    out.write(reinterpret_cast<const char*>(&byteRate), 4);
    out.write(reinterpret_cast<const char*>(&blockAlign), 2);
    out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&dataChunkSize), 4);
}

template<typename Func>
void generateWav(const QString& path, double duration, int sampleRate, Func generatorFn) {
    std::ofstream out(path.toStdString(), std::ios::binary);
    if (!out.is_open()) return;

    int totalSamples = static_cast<int>(sampleRate * duration);
    writeWavHeader(out, totalSamples, sampleRate);

    for (int i = 0; i < totalSamples; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double sample = generatorFn(t, duration, i);
        int val = static_cast<int>(sample * 30000.0);
        int16_t sampleVal = static_cast<int16_t>(std::max(-32767, std::min(32767, val)));
        out.write(reinterpret_cast<const char*>(&sampleVal), 2);
    }
}
} // namespace

AudioManager::AudioManager(QObject* parent)
    : QObject(parent), m_muted(false), m_volume(0.65f)
{
    initAudioAssets();
}

void AudioManager::initAudioAssets() {
    QDir dir("sounds");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    constexpr int sampleRate = 44100;

    // 1. Move Sound
    QString movePath = "sounds/move.wav";
    if (!QFile::exists(movePath)) {
        generateWav(movePath, 0.035, sampleRate, [](double t, double d, int) {
            double freq = 900.0 - (t / d) * 300.0;
            double env = std::exp(-35.0 * t / d);
            return std::sin(2.0 * PI * freq * t) * env * 0.4;
        });
    }
    loadEffect("move", movePath);

    // 2. Rotate Sound
    QString rotPath = "sounds/rotate.wav";
    if (!QFile::exists(rotPath)) {
        generateWav(rotPath, 0.06, sampleRate, [](double t, double d, int) {
            double freq = 600.0 + (t / d) * 450.0;
            double env = std::sin(PI * (t / d));
            return (0.7 * std::sin(2.0 * PI * freq * t) + 0.3 * std::sin(4.0 * PI * freq * t)) * env * 0.5;
        });
    }
    loadEffect("rotate", rotPath);

    // 3. Drop Sound
    QString dropPath = "sounds/drop.wav";
    if (!QFile::exists(dropPath)) {
        static std::mt19937 noiseRng(42);
        static std::uniform_real_distribution<double> noiseDist(-1.0, 1.0);
        generateWav(dropPath, 0.12, sampleRate, [](double t, double d, int) {
            double env = std::exp(-18.0 * t / d);
            double thump = std::sin(2.0 * PI * (140.0 - 70.0 * (t / d)) * t);
            double noise = noiseDist(noiseRng) * 0.35;
            return (thump * 0.7 + noise * 0.3) * env * 0.65;
        });
    }
    loadEffect("drop", dropPath);

    // 4. Lock Sound
    QString lockPath = "sounds/lock.wav";
    if (!QFile::exists(lockPath)) {
        generateWav(lockPath, 0.045, sampleRate, [](double t, double d, int) {
            double env = std::exp(-30.0 * t / d);
            return (std::sin(2.0 * PI * 520.0 * t) + 0.5 * std::sin(2.0 * PI * 1040.0 * t)) * env * 0.45;
        });
    }
    loadEffect("lock", lockPath);

    // 5. Clear Sounds (Combo Levels 1, 2, 3, 4+)
    double rootFreqs[] = { 523.25, 659.25, 783.99, 1046.50 };
    for (int idx = 1; idx <= 4; ++idx) {
        QString clearPath = QString("sounds/clear_%1.wav").arg(idx);
        double root = rootFreqs[idx - 1];
        if (!QFile::exists(clearPath)) {
            generateWav(clearPath, 0.28, sampleRate, [root](double t, double d, int) {
                int seg = static_cast<int>(t / (d / 3.0));
                double mult = (seg == 0) ? 1.0 : ((seg == 1) ? 1.25 : 1.5);
                double freq = root * mult;
                double localT = t - (seg * (d / 3.0));
                double env = std::exp(-12.0 * localT / (d / 3.0));
                double waveVal = (
                    0.7 * std::sin(2.0 * PI * freq * t) +
                    0.25 * std::sin(4.0 * PI * freq * t) +
                    0.1 * std::sin(6.0 * PI * freq * t)
                );
                return waveVal * env * 0.7;
            });
        }
        loadEffect(QString("clear_%1").arg(idx), clearPath);
    }

    // 6. Game Over Sound
    QString goPath = "sounds/game_over.wav";
    if (!QFile::exists(goPath)) {
        generateWav(goPath, 0.6, sampleRate, [](double t, double d, int) {
            int seg = static_cast<int>(t / (d / 3.0));
            double freqs[] = { 440.0, 392.0, 349.23 };
            double freq = freqs[std::min(2, seg)];
            double localT = t - (seg * (d / 3.0));
            double env = std::exp(-8.0 * localT / (d / 3.0));
            return std::sin(2.0 * PI * freq * t) * env * 0.6;
        });
    }
    loadEffect("game_over", goPath);
}

void AudioManager::loadEffect(const QString& name, const QString& filepath) {
    if (!QFile::exists(filepath)) return;
    auto* effect = new QSoundEffect(this);
    effect->setSource(QUrl::fromLocalFile(QFileInfo(filepath).absoluteFilePath()));
    effect->setVolume(m_volume);
    m_effects[name] = effect;
}

void AudioManager::playEffect(const QString& name) {
    if (m_muted) return;
    auto it = m_effects.find(name);
    if (it != m_effects.end() && it.value()) {
        it.value()->play();
    }
}

bool AudioManager::toggleMute() {
    m_muted = !m_muted;
    return m_muted;
}

void AudioManager::setVolume(float volume) {
    m_volume = std::max(0.0f, std::min(1.0f, volume));
    for (auto* eff : m_effects) {
        if (eff) eff->setVolume(m_volume);
    }
}

void AudioManager::playMove() {
    playEffect("move");
}

void AudioManager::playRotate() {
    playEffect("rotate");
}

void AudioManager::playDrop() {
    playEffect("drop");
}

void AudioManager::playLock() {
    playEffect("lock");
}

void AudioManager::playClear(int combo) {
    int c = std::max(1, std::min(4, combo));
    playEffect(QString("clear_%1").arg(c));
}

void AudioManager::playGameOver() {
    playEffect("game_over");
}
