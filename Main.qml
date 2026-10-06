import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    visible: true
    width: 1280
    height: 720
    title: "VenHue"

    StackView {
        id: view
        initialItem: welcomePage1
    }

    // main
    Component {
        id: mainPage
        ColumnLayout {
            spacing: 1

            Label {
                text: "VenHue"
                font.pixelSize: 24
                font.bold: true
            }

            Label {
                text: controller.hueStatusText
            }

            Label {
                text: controller.currentAreaName
            }

            Label {
                text: controller.currentEffect
            }

            Button {
                text: "Settings"
                onClicked: view.push(settingsScreen)
            }
        }
    }

    // settings
    Component {
        id: settingsScreen

        ColumnLayout {
            spacing: 1

            Label {
                text: "Settings"
                font.pixelSize: 24
                font.bold: true
            }

            Button {
                text: "Reconfigure Hue Bridge"
                onClicked: view.push(welcomePage3, { firstTimeSetup: false} )
            }

            Button {
                text: "Back"
                onClicked: view.pop()
            }
        }
    }

    // welcome
    Component {
        id: welcomePage1

        ColumnLayout{
            spacing: 1

            Label{
                text: "Welcome to VenHue!"
                font.pixelSize: 24
                font.bold: true
            }

            Label {
                text: "Choose your platform"
            }

            Button {
                text: "RPCS3"
                onClicked: view.push(welcomePage2)
            }

            Button {
                text: "Xbox 360"
                enabled: false
            }
        }
    }

    // welcome paths
    Component {
        id: welcomePage2

        ColumnLayout {
            spacing: 1

            Label {
                text: "Enter your Rock Band 3 USRDIR path"
                font.pixelSize: 24
                font.bold: true
            }

            Label {
                text: controller.usrdirPathStatus
            }

            TextField {
                placeholderText: controller.getUsrdirPath()

                onTextEdited: controller.setUsrdirPath(text)

            }

            Button {
                text: "Browse..."
                onClicked: controller.browseForUsrdirPath()
            }

            Button {
                text: "Continue"
                enabled: controller.isUsrdirPathValid
                onClicked: view.push(welcomePage3, { firstTimeSetup: true })
            }
            
            Button {
                text: "Back"
                onClicked: view.pop()
            }
            
        }
    }

    // welcome hue setup
    Component {
        id: welcomePage3

        ColumnLayout {
            spacing: 1

            Label {
                text: "Connect to your Hue Bridge"
                font.pixelSize: 24
                font.bold: true
            }

            property bool firstTimeSetup: false

            Label {
                text: controller.hueStatusText
            }

            Label {
                text: "Press the link button on your Hue Bridge."
                visible: controller.hueState === HueConnectionState.AwaitingLink
            }

            Button {
                text: "Connect"
                visible: controller.hueState === HueConnectionState.Unconfigured
                onClicked: controller.connectHue()
            }

            Button {
                text: "Retry"
                visible: controller.hueState === HueConnectionState.Error || controller.hueState === HueConnectionState.NoAreas || controller.hueState === HueConnectionState.Disconnected
                onClicked: controller.retryHueConnection()
            }

            Button {
                text: "Cancel"
                visible: controller.hueState === HueConnectionState.Searching
                    || controller.hueState === HueConnectionState.AwaitingLink
                    || controller.hueState === HueConnectionState.LoadingBridge
                    || controller.hueState === HueConnectionState.ConnectingArea
                    || controller.hueState === HueConnectionState.Reconnecting
                onClicked: controller.cancelHueConnection()
            }
        }
    }
}