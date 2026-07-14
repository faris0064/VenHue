#pragma once
#include "hue_connection_state.h"
#include <QObject>
#include <QString>
#include <QVariantList>

class LightDirector;
class VenueMonitor;
class ConfigurationManager;

// Acts as a bridge between QML and C++, and other functions.
class Controller : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString currentEffect READ currentEffect NOTIFY currentEffectChanged)
    Q_PROPERTY(QString usrdirPathStatus READ usrdirPathStatus NOTIFY usrdirPathStatusChanged)
    Q_PROPERTY(bool isUsrdirPathValid READ isUsrdirPathValid NOTIFY usrdirPathValidChanged)
    Q_PROPERTY(bool baseOnboardingComplete READ baseOnboardingComplete NOTIFY baseOnboardingCompleteChanged)
    Q_PROPERTY(HueConnectionState::HueState hueState READ hueState NOTIFY hueStateChanged)
    Q_PROPERTY(QString hueStatusText READ hueStatusText NOTIFY hueStatusTextChanged)
    Q_PROPERTY(QVariantList entertainmentAreas READ entertainmentAreas NOTIFY entertainmentAreasChanged)
    Q_PROPERTY(QString currentAreaId READ currentAreaId NOTIFY currentAreaChanged)
    Q_PROPERTY(QString currentAreaName READ currentAreaName NOTIFY currentAreaChanged)

public:
    explicit Controller(QObject *parent = nullptr);
    ~Controller();

    // Getters
    QString currentEffect() const;
    QString usrdirPathStatus() const;
    bool isUsrdirPathValid() const;
    bool baseOnboardingComplete() const;
    HueConnectionState::HueState hueState() const;
    QString hueStatusText() const;
    QVariantList entertainmentAreas() const;
    QString currentAreaId() const;
    QString currentAreaName() const;

    // LightDirector
    Q_INVOKABLE void initializeHue();
    Q_INVOKABLE void connectHue();
    Q_INVOKABLE void cancelHueConnection();
    Q_INVOKABLE void retryHueConnection();
    Q_INVOKABLE void selectEntertainmentArea(const QString &id);
    Q_INVOKABLE void resetAllHueData();

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
    void usrdirPathStatusChanged();
    void usrdirPathValidChanged();
    void baseOnboardingCompleteChanged();
    void hueStateChanged();
    void hueStatusTextChanged();
    void entertainmentAreasChanged();
    void currentAreaChanged();

private:
    void setupConnections();

    LightDirector *lightDirector;
    VenueMonitor *venueMonitor;
    ConfigurationManager *configManager;
};
