#pragma once
#include <QObject>
#include <QString>

class LightDirector;
class VenueMonitor;
class ConfigurationManager;

// Acts as a bridge between QML and C++, and other functions.
class Controller : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString currentEffect READ currentEffect NOTIFY currentEffectChanged)
    Q_PROPERTY(QString bridgeStatus READ bridgeStatus NOTIFY bridgeStatusChanged)
    Q_PROPERTY(bool bridgeFound READ bridgeFound NOTIFY bridgeFoundChanged)
    Q_PROPERTY(QString usrdirPathStatus READ usrdirPathStatus NOTIFY usrdirPathStatusChanged)
    Q_PROPERTY(bool isUsrdirPathValid READ isUsrdirPathValid NOTIFY usrdirPathValidChanged)
    Q_PROPERTY(bool hasAvailableAreas READ hasAvailableAreas NOTIFY areasAvailableChanged)
    Q_PROPERTY(QString currentArea READ currentArea NOTIFY areaSelected)

public:
    explicit Controller(QObject *parent = nullptr);
    ~Controller();

    // Getters
    QString currentEffect() const;
    QString bridgeStatus() const;
    bool bridgeFound() const;
    QString usrdirPathStatus() const;
    bool isUsrdirPathValid() const;
    bool hasAvailableAreas() const;
    QString currentArea() const;

    // LightDirector
    Q_INVOKABLE bool isBridgePaired() const;
    Q_INVOKABLE bool isStreaming() const;
    Q_INVOKABLE void searchForBridge();
    Q_INVOKABLE void connectToBridge();
    Q_INVOKABLE void resetBridgePairing();

    Q_INVOKABLE QStringList getAvailableAreas() const;
    Q_INVOKABLE void selectArea(const QString &areaName);

    // ConfigurationManager
    Q_INVOKABLE QString getUsrdirPath() const;
    Q_INVOKABLE void setUsrdirPath(const QString &path);
    Q_INVOKABLE void browseForUsrdirPath();
    
    Q_INVOKABLE QString getPlatform() const;
    Q_INVOKABLE void setPlatform(const QString &platform);


public slots:
    void startCueMonitor();

signals:
    void currentEffectChanged();
    void bridgeStatusChanged();
    void bridgeFoundChanged();
    void bridgePaired();
    void usrdirPathStatusChanged();
    void usrdirPathValidChanged();
    void areasAvailableChanged();
    void areaSelected();

private:
    void setupConnections();

    LightDirector *lightDirector;
    VenueMonitor *venueMonitor;
    ConfigurationManager *configManager;
};
