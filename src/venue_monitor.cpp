#include "include/venue_monitor.h"
#include "include/logger.h"
#include <QDir>
#include <QFile>
#include <fstream>

VenueMonitor::VenueMonitor(QObject *parent)
    : QObject(parent), monitorTimer(nullptr), lastElapsed(-1.0), wasPlaying(false), m_currentEffect("Idle...") {
    monitorTimer = new QTimer(this);
    monitorTimer->setInterval(8);
    monitorTimer->setTimerType(Qt::PreciseTimer);

    connect(monitorTimer, &QTimer::timeout, this, &VenueMonitor::onMonitorTimer);
}

void VenueMonitor::startMonitoring() {
    if (monitorTimer && !monitorTimer->isActive()) {
        monitorTimer->start();
        Logger::info("VenueMonitor: Started monitoring");
    }
}

void VenueMonitor::stopMonitoring() {
    if (monitorTimer && monitorTimer->isActive()) {
        monitorTimer->stop();
        Logger::info("VenueMonitor: Stopped monitoring");
    }
}

void VenueMonitor::setSongStatusPath(const std::string &path) {
    songStatusPath = path;
}

void VenueMonitor::setLightingFilePath(const std::string &path) {
    lightingFilePath = path;
}

void VenueMonitor::onMonitorTimer() {
    bool isPlaying = isSongPlaying();

    if (isPlaying != wasPlaying) {
        if (isPlaying) {
            lastKnownData = VenueData();
            lastElapsed = -1.0;
        }
        else if (m_currentEffect != "Idle...") {
            m_currentEffect = "Idle...";
            emit currentEffectChanged(m_currentEffect);
            Logger::info("Idle...");
        }
        wasPlaying = isPlaying;
        emit songStateChanged(isPlaying);
    }

    if (!isPlaying) {
        return;
    }

    VenueData newData;
    try {
        if (!lightingFilePath.empty()) {
            newData = parser.parseDataFile(lightingFilePath);

            if (!newData.timeline.empty()) {
                lastKnownData = newData;

                if (std::abs(newData.currentElapsed - lastElapsed) > 0.0001) {
                    printActiveEffect(newData, newData.currentElapsed);
                    lastElapsed = newData.currentElapsed;
                }
            }
            else {
                handleLightingFileError();
            }
        }
    } catch (...) {
        handleLightingFileError();
    }
}

bool VenueMonitor::isSongPlaying() {
    if (songStatusPath.empty()) {
        return false;
    }

    std::ifstream file(songStatusPath);
    if (!file.is_open()) {
        return false;
    }

    // The JSON should be empty if no song is playing
    char firstChar;
    if (!(file >> firstChar)) {
        return false;
    }

    return true;
}

void VenueMonitor::handleLightingFileError() {
    if (isSongPlaying() && !lastKnownData.timeline.empty()) {
        printActiveEffect(lastKnownData, lastKnownData.currentElapsed);
    }
}

void VenueMonitor::printActiveEffect(const VenueData &data, double currentTime) {
    std::string activeEffect = parser.findActiveEffect(data, currentTime);

    QString newEffect = activeEffect.empty() ? "Idle..." : QString::fromStdString(activeEffect);
    if (newEffect == m_currentEffect) {
        return;
    }
    m_currentEffect = newEffect;
    emit currentEffectChanged(m_currentEffect);

    // For LightDirector
    if (!activeEffect.empty()) {
        emit effectChanged(activeEffect, data, currentTime);
    }

    Logger::info(activeEffect.empty() ? "Idle..." : activeEffect);
}
