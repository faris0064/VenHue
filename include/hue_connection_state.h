#pragma once

#include <QObject>
#include <string>
#include <vector>

struct EntertainmentArea {
    std::string id;
    std::string name;

    bool operator==(const EntertainmentArea &other) const {
        return id == other.id && name == other.name;
    }
};

using EntertainmentAreas = std::vector<EntertainmentArea>;

class HueConnectionState : public QObject {
    Q_OBJECT

public:
    enum HueState {
        Initializing,
        Unconfigured,
        Searching,
        AwaitingLink,
        LoadingBridge,
        SelectingArea,
        ConnectingArea,
        Reconnecting,
        Streaming,
        NoAreas,
        Disconnected,
        Error,
        Resetting
    };
    Q_ENUM(HueState)

    enum RetryAction {
        RetryNone,
        RetryConnect,
        RetryReconnect,
        RetrySelection,
        RetryReset
    };
    Q_ENUM(RetryAction)

    explicit HueConnectionState(QObject *parent = nullptr);

    HueState hueState() const { return m_state; }
    const std::string &hueStatusText() const { return m_statusText; }
    const EntertainmentAreas &entertainmentAreas() const { return m_areas; }
    const std::string &currentAreaId() const { return m_currentAreaId; }
    const std::string &currentAreaName() const { return m_currentAreaName; }
    const std::string &pendingAreaId() const { return m_pendingAreaId; }
    RetryAction retryAction() const { return m_retryAction; }

    void beginInitialization();
    void beginConnect(bool reconnecting);
    void bridgeLoaded(bool configured);
    void searching();
    void awaitingLink();
    void loadingBridge();
    void updateAreas(const EntertainmentAreas &areas);
    void configurationRetrieved(const EntertainmentAreas &areas, bool actionRequired);
    void requireAreaSelection();
    void beginAreaSelection(const std::string &id);
    void loadConfirmedArea(const std::string &areaId, const std::string &areaName);
    void activatingArea();
    void confirmStreaming(const std::string &areaId, const std::string &areaName);
    void disconnected();
    void fail(const std::string &message, RetryAction retryAction);
    void cancel();
    void beginReset();
    void finishReset();

signals:
    void hueStateChanged();
    void hueStatusTextChanged();
    void entertainmentAreasChanged();
    void currentAreaChanged();

private:
    void setState(HueState state, const std::string &statusText);
    void clearAreas();

    HueState m_state = Initializing;
    std::string m_statusText = "Loading previous setup...";
    EntertainmentAreas m_areas;
    std::string m_currentAreaId;
    std::string m_currentAreaName;
    std::string m_pendingAreaId;
    RetryAction m_retryAction = RetryNone;
};
