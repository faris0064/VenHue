#pragma once
#include <QObject>
#include <QSettings>
#include <QString>

class ConfigurationManager : public QObject {
    Q_OBJECT

public:
    explicit ConfigurationManager(QObject *parent = nullptr);

    QString getUsrdirPath() const;
    void setUsrdirPath(const QString &path);
    void browseForUsrdirPath();
    bool isUsrdirPathValid() const { return m_isUsrdirPathValid; }
    QString getUsrdirPathStatus() const { return m_usrdirPathStatus; }

    QString getPlatform() const;
    void setPlatform(const QString &platform);

    std::string getSongStatusPath() const;
    std::string getLightingFilePath() const;

signals:
    void usrdirPathStatusChanged(const QString &status);
    void usrdirPathValidChanged(bool isValid);
    void pathsUpdated(const std::string &songStatusPath, const std::string &lightingFilePath);

private:
    void validatePath(const QString &path);
    void saveSettings();
    void loadSettings();
    void updatePaths();

    QString m_usrdirPath;
    QString m_usrdirPathStatus;
    bool m_isUsrdirPathValid;
    QString m_platform;
};
