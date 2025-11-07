#include "include/configuration_manager.h"
#include "include/logger.h"
#include <QDir>
#include <QFile>
#include <QtWidgets/QFileDialog>

ConfigurationManager::ConfigurationManager(QObject *parent)
    : QObject(parent), m_isUsrdirPathValid(false) {
    loadSettings();
}

QString ConfigurationManager::getUsrdirPath() const {
    return m_usrdirPath;
}

void ConfigurationManager::setUsrdirPath(const QString &path) {
    m_usrdirPath = path;
    validatePath(path);
    updatePaths();
}

void ConfigurationManager::browseForUsrdirPath() {
    QString directory = QFileDialog::getExistingDirectory(nullptr, tr("Select USRDIR Directory"), QDir::currentPath(), QFileDialog::ShowDirsOnly);
    setUsrdirPath(directory);
}

std::string ConfigurationManager::getSongStatusPath() const {
    if (m_usrdirPath.isEmpty()) {
        return "";
    }
    return (m_usrdirPath + "/currentsong.json").toStdString();
}

std::string ConfigurationManager::getLightingFilePath() const {
    if (m_usrdirPath.isEmpty()) {
        return "";
    }
    return (m_usrdirPath + "/dx_lighting.txt").toStdString();
}

void ConfigurationManager::validatePath(const QString &path) {
    bool isValid = false;
    QString statusMessage = "";

    if (path.isEmpty()) {
        statusMessage = "";
        isValid = false;
    }
    else {
        QDir dir(path);
        if (!dir.exists()) {
            statusMessage = "Path does not exist";
            isValid = false;
        }
        else {
            QFile songStatusFile(path + "/currentsong.json");
            QFile lightingFile(path + "/dx_lighting.txt");

            if (songStatusFile.exists() || lightingFile.exists()) {
                statusMessage = "Valid Rock Band 3 USRDIR folder found";
                isValid = true;
            }
            else {
                statusMessage = "This doesn't appear to be a Rock Band 3 USRDIR folder. Make sure you've ran Rock Band 3 Deluxe at least once.";
                isValid = false;
            }
        }
    }

    if (m_usrdirPathStatus != statusMessage) {
        m_usrdirPathStatus = statusMessage;
        emit usrdirPathStatusChanged(statusMessage);
    }

    if (m_isUsrdirPathValid != isValid) {
        m_isUsrdirPathValid = isValid;
        emit usrdirPathValidChanged(isValid);
    }

    if (isValid) {
        saveSettings();
    }
}

QString ConfigurationManager::getPlatform() const {
    return m_platform;
}

void ConfigurationManager::setPlatform(const QString &platform) {
    if (m_platform != platform) {
        m_platform = platform;
        saveSettings();
        Logger::info("ConfigurationManager: Platform set to " + platform.toStdString());
    }
}

void ConfigurationManager::saveSettings() {
    QSettings settings;
    settings.setValue("rockband3/usrdir_path", m_usrdirPath);
    settings.setValue("general/platform", m_platform);
    Logger::info("ConfigurationManager: Saved settings to storage");
}

void ConfigurationManager::loadSettings() {
    QSettings settings;
    QString savedPath = settings.value("rockband3/usrdir_path", "").toString();
    m_platform = settings.value("general/platform", "").toString();

    if (!savedPath.isEmpty()) {
        m_usrdirPath = savedPath;
        validatePath(savedPath);
        updatePaths();
        Logger::info("ConfigurationManager: Loaded USRDIR path from settings");
    }
    
    if (!m_platform.isEmpty()) {
        Logger::info("ConfigurationManager: Loaded platform: " + m_platform.toStdString());
    }
}

void ConfigurationManager::updatePaths() {
    if (m_isUsrdirPathValid) {
        std::string songStatusPath = getSongStatusPath();
        std::string lightingFilePath = getLightingFilePath();
        emit pathsUpdated(songStatusPath, lightingFilePath);
        Logger::info("ConfigurationManager: Updated file paths");
    }
}
