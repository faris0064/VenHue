#include "hue_connection_state.h"

HueConnectionState::HueConnectionState(QObject *parent)
    : QObject(parent) {}

void HueConnectionState::setState(HueState state, const std::string &statusText) {
    if (m_state != state) {
        m_state = state;
        emit hueStateChanged();
    }
    if (m_statusText != statusText) {
        m_statusText = statusText;
        emit hueStatusTextChanged();
    }
}

void HueConnectionState::clearAreas() {
    if (!m_areas.empty()) {
        m_areas.clear();
        emit entertainmentAreasChanged();
    }
}

void HueConnectionState::beginInitialization() {
    m_retryAction = RetryNone;
    setState(Initializing, "Loading previous setup...");
}

void HueConnectionState::beginConnect(bool reconnecting) {
    m_pendingAreaId.clear();
    m_retryAction = reconnecting ? RetryReconnect : RetryConnect;
    setState(reconnecting ? Reconnecting : Searching,
             reconnecting ? "Reconnecting to Hue Bridge..."
                          : "Searching for a Hue bridge...");
}

void HueConnectionState::bridgeLoaded(bool configured) {
    if (configured) {
        m_retryAction = RetryReconnect;
        setState(Reconnecting, "Previous bridge found, reconnecting...");
    }
    else {
        m_retryAction = RetryNone;
        clearAreas();
        setState(Unconfigured, "Connect a Hue Bridge.");
    }
}

void HueConnectionState::searching() {
    setState(Searching, "Searching for a Hue bridge...");
}

void HueConnectionState::awaitingLink() {
    setState(AwaitingLink, "Press the link button on your Hue bridge.");
}

void HueConnectionState::loadingBridge() {
    switch (m_state) {
        case Initializing:
        case Searching:
        case AwaitingLink:
        case LoadingBridge:
        case Reconnecting:
            setState(LoadingBridge, "Loading bridge and Entertainment Areas...");
            break;
        default:
            break;
    }
}

void HueConnectionState::updateAreas(const EntertainmentAreas &areas) {
    if (m_areas != areas) {
        m_areas = areas;
        emit entertainmentAreasChanged();
    }

    if (m_areas.empty() && (m_state == SelectingArea || m_state == LoadingBridge || m_state == NoAreas)) {
        m_retryAction = RetryReconnect;
        setState(NoAreas, "No Entertainment Areas were found. Create one in the Philips Hue app, then retry.");
    }
    else if (!m_areas.empty() && m_state == NoAreas) {
        if (m_areas.size() == 1) {
            setState(ConnectingArea, "Connecting to Entertainment Area...");
        }
        else {
            requireAreaSelection();
        }
    }
}

void HueConnectionState::configurationRetrieved(const EntertainmentAreas &areas, bool actionRequired) {
    updateAreas(areas);
    if (areas.empty()) {
        m_retryAction = RetryReconnect;
        setState(NoAreas, "No Entertainment Areas were found. Create one in the Philips Hue app, then retry.");
    }
    else if (actionRequired && areas.size() > 1) {
        requireAreaSelection();
    }
    else {
        setState(ConnectingArea, "Starting streaming...");
    }
}

void HueConnectionState::requireAreaSelection() {
    if (m_areas.empty()) {
        m_retryAction = RetryReconnect;
        setState(NoAreas, "No Entertainment Areas were found. Create one in the Philips Hue app, then retry.");
        return;
    }
    m_retryAction = RetrySelection;
    setState(SelectingArea, "Choose the Entertainment Area VenHue should use.");
}

void HueConnectionState::beginAreaSelection(const std::string &id) {
    m_pendingAreaId = id;
    m_retryAction = RetrySelection;
    setState(ConnectingArea, "Connecting to Entertainment Area...");
}

void HueConnectionState::loadConfirmedArea(const std::string &areaId, const std::string &areaName) {
    if (areaId.empty()) {
        return;
    }
    const bool changed = m_currentAreaId != areaId || m_currentAreaName != areaName;
    m_currentAreaId = areaId;
    m_currentAreaName = areaName;
    if (changed) {
        emit currentAreaChanged();
    }
}

void HueConnectionState::activatingArea() {
    setState(ConnectingArea, "Starting streaming...");
}

void HueConnectionState::confirmStreaming(const std::string &areaId, const std::string &areaName) {
    const bool changed = m_currentAreaId != areaId || m_currentAreaName != areaName;
    m_currentAreaId = areaId;
    m_currentAreaName = areaName;
    m_pendingAreaId.clear();
    m_retryAction = RetryReconnect;
    if (changed) {
        emit currentAreaChanged();
    }
    setState(Streaming, "Streaming to " + areaName + ".");
}

void HueConnectionState::disconnected() {
    m_pendingAreaId.clear();
    m_retryAction = RetryReconnect;
    setState(Disconnected, "Streaming disconnected.");
}

void HueConnectionState::fail(const std::string &message, RetryAction retryAction) {
    m_pendingAreaId.clear();
    m_retryAction = retryAction;
    setState(Error, message);
}

void HueConnectionState::cancel() {
    m_pendingAreaId.clear();
    m_retryAction = RetryNone;
    if (m_currentAreaId.empty()) {
        clearAreas();
        setState(Unconfigured, "Hue setup cancelled.");
    }
    else {
        setState(Disconnected, "Hue connection cancelled.");
    }
}

void HueConnectionState::beginReset() {
    m_pendingAreaId.clear();
    m_retryAction = RetryReset;
    setState(Resetting, "Unpairing from Hue Bridge.");
}

void HueConnectionState::finishReset() {
    const bool areaChanged = !m_currentAreaId.empty() || !m_currentAreaName.empty();
    m_currentAreaId.clear();
    m_currentAreaName.clear();
    m_pendingAreaId.clear();
    clearAreas();
    m_retryAction = RetryNone;
    if (areaChanged) {
        emit currentAreaChanged();
    }
    setState(Unconfigured, "Bridge unpaired.");
}
