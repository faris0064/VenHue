import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    visible: true
    width: 1280
    height: 720
    title: "VenHue"

    // CONSTANTS
    readonly property real kMenuLeftFraction: 0.07
    readonly property real kMenuTopFraction: 0.18
    readonly property real kMenuWidthFraction: 0.4


    StackView {
        id: view
        initialItem: controller.baseOnboardingComplete ? mainPage : welcomePage1
        anchors.fill: parent
    }

    // main
    Component {
        id: mainPage

        Item {
            ColumnLayout {
                spacing: 2
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.leftMargin: parent.width * kMenuLeftFraction
                anchors.topMargin: parent.height * kMenuTopFraction
                width: parent.width * kMenuWidthFraction

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
    }

    // settings
    Component {
        id: settingsScreen

        Item {

        
            ColumnLayout {
                spacing: 2
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.leftMargin: parent.width * kMenuLeftFraction
                anchors.topMargin: parent.height * kMenuTopFraction
                width: parent.width * kMenuWidthFraction

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
    }

    // welcome

    Component {
        id: welcomePage1

        Item {

            ColumnLayout {
                spacing: 2
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.leftMargin: parent.width * kMenuLeftFraction
                anchors.topMargin: parent.height * kMenuTopFraction
                width: parent.width * kMenuWidthFraction

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
                    onClicked: { 
                        controller.setPlatform("RPCS3")
                        view.push(welcomePage2)
                    }
                }

                Button {
                    text: "Xbox 360"
                    enabled: false
                }
            }
        }
    }


    // welcome paths
    Component {
        id: welcomePage2

        Item {

            ColumnLayout {
                spacing: 2
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.leftMargin: parent.width * kMenuLeftFraction
                anchors.topMargin: parent.height * kMenuTopFraction
                width: parent.width * kMenuWidthFraction

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
    }

    // welcome hue setup
    Component {
        id: welcomePage3

        Item {

            property bool firstTimeSetup: false
        
            ColumnLayout {
                spacing: 2
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.leftMargin: parent.width * kMenuLeftFraction
                anchors.topMargin: parent.height * kMenuTopFraction
                width: parent.width * kMenuWidthFraction

                Label {
                    text: "Connect to your Hue Bridge"
                    font.pixelSize: 24
                    font.bold: true
                }


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

                ComboBox {
                    visible: controller.hueState === HueConnectionState.SelectingArea || controller.hueState === HueConnectionState.ConnectingArea
                    enabled: controller.hueState === HueConnectionState.SelectingArea
                    model: controller.entertainmentAreas
                    textRole: "name"
                    valueRole: "id"
                    currentIndex: -1
                    displayText: currentIndex === -1 ? "Select an Entertainment Area" : currentText

                    onActivated: controller.selectEntertainmentArea(currentValue)
                }

                Button {
                    text: firstTimeSetup ? "Finish" : "Done"
                    visible: controller.hueState === HueConnectionState.SelectingArea 
                            || controller.hueState === HueConnectionState.ConnectingArea
                            || controller.hueState === HueConnectionState.Streaming
                    enabled: controller.hueState === HueConnectionState.Streaming

                    onClicked: firstTimeSetup ? view.replace(null, mainPage) : view.pop()
                }
            }
        }
    }
}