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

QString Controller::bridgeStatus() const {
    return lightDirector->bridgeStatus();
}

bool Controller::bridgeFound() const {
    return lightDirector->bridgeFound();
}

QString Controller::usrdirPathStatus() const {
    return configManager->getUsrdirPathStatus();
}

bool Controller::isUsrdirPathValid() const {
    return configManager->isUsrdirPathValid();
}

bool Controller::hasAvailableAreas() const {
    return lightDirector->hasAvailableAreas();
}

QString Controller::currentArea() const {
    return lightDirector->getCurrentArea();
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
    configManager->setPlatform(platform);
}

bool Controller::isBridgePaired() const {
    return lightDirector->isBridgePaired();
}

bool Controller::isStreaming() const {
    return lightDirector->isStreaming();
}

void Controller::searchForBridge() {
    lightDirector->searchForBridge();
}

void Controller::connectToBridge() {
    lightDirector->connectToBridge();
}

void Controller::resetBridgePairing() {
    lightDirector->resetBridgePairing();
}

QStringList Controller::getAvailableAreas() const {
    return lightDirector->getAvailableAreas();
}

void Controller::selectArea(const QString &areaName) {
    lightDirector->selectArea(areaName);
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
    connect(lightDirector, &LightDirector::bridgeStatusChanged,
            this, &Controller::bridgeStatusChanged);
    connect(lightDirector, &LightDirector::bridgeFoundChanged,
            this, &Controller::bridgeFoundChanged);
    connect(lightDirector, &LightDirector::bridgePaired,
            this, &Controller::bridgePaired);
    connect(lightDirector, &LightDirector::areasAvailableChanged,
            this, &Controller::areasAvailableChanged);
    connect(lightDirector, &LightDirector::areaSelected,
            this, &Controller::areaSelected);

    // ConfigurationManager signals
    connect(configManager, &ConfigurationManager::usrdirPathStatusChanged,
            this, &Controller::usrdirPathStatusChanged);
    connect(configManager, &ConfigurationManager::usrdirPathValidChanged,
            this, &Controller::usrdirPathValidChanged);
    connect(configManager, &ConfigurationManager::pathsUpdated,
            [this](const std::string &songStatusPath, const std::string &lightingFilePath) {
                venueMonitor->setSongStatusPath(songStatusPath);
                venueMonitor->setLightingFilePath(lightingFilePath);
            });
}
