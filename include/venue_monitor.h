#pragma once
#include "include/parser.h"
#include <QObject>
#include <QString>
#include <QTimer>

class VenueMonitor : public QObject {
    Q_OBJECT

public:
    explicit VenueMonitor(QObject *parent = nullptr);

    void startMonitoring();
    void stopMonitoring();
    QString getCurrentEffect() const { return m_currentEffect; }

    void setSongStatusPath(const std::string &path);
    void setLightingFilePath(const std::string &path);

signals:
    void effectChanged(const std::string &effectName, const VenueData &data, double currentTime);
    void currentEffectChanged(const QString &effect);
    void songStateChanged(bool isPlaying);

private slots:
    void onMonitorTimer();

private:
    bool isSongPlaying();
    void handleLightingFileError();
    void printActiveEffect(const VenueData &data, double currentTime);

    Parser parser;
    QTimer *monitorTimer;
    double lastElapsed;
    bool wasPlaying;
    VenueData lastKnownData;
    QString m_currentEffect;

    std::string songStatusPath;
    std::string lightingFilePath;
};
