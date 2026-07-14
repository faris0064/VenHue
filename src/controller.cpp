#include "include/controller.h"
#include "include/configuration_manager.h"
#include "include/light_director.h"
#include "include/logger.h"
#include "include/venue_monitor.h"

Controller::Controller(QObject *parent)
    : QObject(parent) {

    lightDirector = new LightDirector(this);
    venueMonitor = new VenueMonitor(this);
    configManager = new ConfigurationManager(this);

    setupConnections();
}

Controller::~Controller() {
    // Clean up streaming connection before exit
    if (lightDirector) {
        lightDirector->shutdown();
    }
    Logger::info("Controller: Shutting down");
}

QString Controller::currentEffect() const {
    return venueMonitor->getCurrentEffect();
}

QString Controller::usrdirPathStatus() const {
    return configManager->getUsrdirPathStatus();
}

bool Controller::isUsrdirPathValid() const {
    return configManager->isUsrdirPathValid();
}

bool Controller::baseOnboardingComplete() const {
    return configManager->isUsrdirPathValid() && !configManager->getPlatform().isEmpty();
}

HueConnectionState::HueState Controller::hueState() const {
    return lightDirector->connectionState()->hueState();
}

QString Controller::hueStatusText() const {
    return QString::fromStdString(lightDirector->connectionState()->hueStatusText());
}

QVariantList Controller::entertainmentAreas() const {
    QVariantList areas;
    for (const auto &area : lightDirector->connectionState()->entertainmentAreas()) {
        QVariantMap value;
        value.insert(QStringLiteral("id"), QString::fromStdString(area.id));
        value.insert(QStringLiteral("name"), QString::fromStdString(area.name));
        areas.append(value);
    }
    return areas;
}

QString Controller::currentAreaId() const {
    return QString::fromStdString(lightDirector->connectionState()->currentAreaId());
}

QString Controller::currentAreaName() const {
    return QString::fromStdString(lightDirector->connectionState()->currentAreaName());
}

void Controller::initializeHue() {
    lightDirector->initializeHue();
}

void Controller::connectHue() {
    lightDirector->connectHue();
}

void Controller::cancelHueConnection() {
    lightDirector->cancelHueConnection();
}

void Controller::retryHueConnection() {
    lightDirector->retryHueConnection();
}

void Controller::selectEntertainmentArea(const QString &id) {
    lightDirector->selectEntertainmentArea(id.toStdString());
}

void Controller::resetAllHueData() {
    lightDirector->resetAllHueData();
}

QString Controller::getUsrdirPath() const {
    return configManager->getUsrdirPath();
}

void Controller::setUsrdirPath(const QString &path) {
    configManager->setUsrdirPath(path);
}

void Controller::browseForUsrdirPath() {
    configManager->browseForUsrdirPath();
}

QString Controller::getPlatform() const {
    return configManager->getPlatform();
}

void Controller::setPlatform(const QString &platform) {
    const bool wasComplete = baseOnboardingComplete();
    configManager->setPlatform(platform);
    if (wasComplete != baseOnboardingComplete()) {
        emit baseOnboardingCompleteChanged();
    }
}

void Controller::startCueMonitor() {
    venueMonitor->startMonitoring();
}

void Controller::setupConnections() {
    // VenueMonitor signals
    connect(venueMonitor, &VenueMonitor::currentEffectChanged,
            this, &Controller::currentEffectChanged);
    connect(venueMonitor, &VenueMonitor::effectChanged,
            lightDirector, &LightDirector::onEffectChanged);

    // LightDirector signals 
    auto state = lightDirector->connectionState();
    connect(state, &HueConnectionState::hueStateChanged,
            this, &Controller::hueStateChanged);
    connect(state, &HueConnectionState::hueStatusTextChanged,
            this, &Controller::hueStatusTextChanged);
    connect(state, &HueConnectionState::entertainmentAreasChanged,
            this, &Controller::entertainmentAreasChanged);
    connect(state, &HueConnectionState::currentAreaChanged,
            this, &Controller::currentAreaChanged);

    // ConfigurationManager signals
    connect(configManager, &ConfigurationManager::usrdirPathStatusChanged,
            this, &Controller::usrdirPathStatusChanged);
    connect(configManager, &ConfigurationManager::usrdirPathValidChanged,
            this, [this](bool) {
                emit usrdirPathValidChanged();
                emit baseOnboardingCompleteChanged();
            });
    connect(configManager, &ConfigurationManager::pathsUpdated,
            [this](const std::string &songStatusPath, const std::string &lightingFilePath) {
                venueMonitor->setSongStatusPath(songStatusPath);
                venueMonitor->setLightingFilePath(lightingFilePath);
            });
}
