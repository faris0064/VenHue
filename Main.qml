import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

ApplicationWindow {
    id: window
    visible: true
    width: 500
    height: 700
    title: "VenHue"
    
    // COLOR PALETTE 
    readonly property QtObject colors: QtObject {
        // Background colors
        readonly property color background: "#1a1a1a"           
        readonly property color cardBackground: "#2a2a2a"      
        readonly property color cardBorder: "#3a3a3a"          
        readonly property color inputBackground: "#353535"     
        
        // Accent colors
        readonly property color primary: "#a0db2e"             
        readonly property color secondary: "#6a6a6a"         
        readonly property color accent: "#808080"            
        
        // Text colors
        readonly property color textPrimary: "#ffffff"       
        readonly property color textSecondary: "#999999"      
        readonly property color textTertiary: "#666666"       
        
        // Status colors
        readonly property color success: "#a0db2e"             
        readonly property color successBg: "#2d3d24"           
        readonly property color successBorder: "#4d5d34"       
        readonly property color error: "#ff6b6b"               
        readonly property color errorBg: "#3d2424"             
        readonly property color errorBorder: "#5d3434"        
        readonly property color disabled: "#3a3a3a"            
    }
    
    color: colors.background
    
    // Keep track of whether onboarding is complete
    property bool onboardingComplete: false
    property string selectedPlatform: ""
    
    // Load saved settings on startup
    Component.onCompleted: {
        var savedPlatform = controller.getPlatform();
        if (savedPlatform !== "") {
            selectedPlatform = savedPlatform;
        }
        
        if (controller.isBridgePaired()) {
            onboardingComplete = true;
        }
    }
    
    // Custom styled button component
    component StyledButton: Rectangle {
        id: styledBtn
        property alias text: btnText.text
        property bool primary: false
        property bool enabled: true
        signal clicked()
        
        width: 200
        height: 50
        radius: 25
        color: {
            if (!enabled) return colors.disabled
            if (primary) return colors.primary
            return colors.secondary
        }
        
        Label {
            id: btnText
            anchors.centerIn: parent
            color: styledBtn.primary ? "#000000" : colors.textPrimary
            font.pixelSize: 16
            font.weight: Font.Medium
        }
        
        MouseArea {
            anchors.fill: parent
            enabled: styledBtn.enabled
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: styledBtn.clicked()
            
            onPressed: parent.opacity = 0.8
            onReleased: parent.opacity = 1.0
        }
    }
    
    // Custom card component
    component Card: Rectangle {
        color: colors.cardBackground
        radius: 16
        border.color: colors.cardBorder
        border.width: 1
        
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#000000"
            shadowBlur: 0.4
            shadowVerticalOffset: 4
            shadowHorizontalOffset: 0
        }
    }
    
    // StackView for navigation
    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: onboardingComplete ? mainPage : welcomePage
        
        // Slide transitions
        pushEnter: Transition {
            PropertyAnimation {
                property: "x"
                from: stackView.width
                to: 0
                duration: 250
                easing.type: Easing.OutCubic
            }
        }
        pushExit: Transition {
            PropertyAnimation {
                property: "x"
                from: 0
                to: -stackView.width * 0.3
                duration: 250
                easing.type: Easing.OutCubic
            }
        }
        popEnter: Transition {
            PropertyAnimation {
                property: "x"
                from: -stackView.width * 0.3
                to: 0
                duration: 250
                easing.type: Easing.OutCubic
            }
        }
        popExit: Transition {
            PropertyAnimation {
                property: "x"
                from: 0
                to: stackView.width
                duration: 250
                easing.type: Easing.OutCubic
            }
        }
    }
    
    // Welcome Page
    Component {
        id: welcomePage
        
        Rectangle {
            width: stackView.width
            height: stackView.height
            color: colors.background
            
            ColumnLayout {
                anchors.centerIn: parent
                width: parent.width * 0.85
                spacing: 40
                
                // App icon placeholder
                Rectangle {
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 80
                    Layout.alignment: Qt.AlignHCenter
                    radius: 20
                    color: colors.cardBackground
                    border.color: colors.primary
                    border.width: 2
                    
                    Label {
                        anchors.centerIn: parent
                        text: "💡"
                        font.pixelSize: 40
                    }
                }
                
                Label {
                    text: "Welcome to VenHue"
                    font.pixelSize: 32
                    font.weight: Font.Bold
                    color: colors.textPrimary
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Label {
                    text: "Choose your platform to get started"
                    font.pixelSize: 16
                    color: colors.textSecondary
                    Layout.alignment: Qt.AlignHCenter
                }
                
                // Platform cards
                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 20
                    
                    // RPCS3 Card
                    Card {
                        Layout.preferredWidth: 160
                        Layout.preferredHeight: 180
                        
                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 15
                            
                            Rectangle {
                                Layout.preferredWidth: 80
                                Layout.preferredHeight: 80
                                Layout.alignment: Qt.AlignHCenter
                                radius: 12
                                color: colors.cardBorder
                                
                                Label {
                                    anchors.centerIn: parent
                                    text: "🎮"
                                    font.pixelSize: 36
                                }
                            }
                            
                            Label {
                                text: "RPCS3"
                                font.pixelSize: 18
                                font.weight: Font.Medium
                                color: colors.textPrimary
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                selectedPlatform = "RPCS3"
                                controller.setPlatform("RPCS3")
                                stackView.push(usrdirPathPage)
                            }
                            onPressed: parent.opacity = 0.8
                            onReleased: parent.opacity = 1.0
                        }
                    }
                    
                    // Xbox 360 Card (disabled)
                    Card {
                        Layout.preferredWidth: 160
                        Layout.preferredHeight: 180
                        opacity: 0.4
                        
                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 15
                            
                            Rectangle {
                                Layout.preferredWidth: 80
                                Layout.preferredHeight: 80
                                Layout.alignment: Qt.AlignHCenter
                                radius: 12
                                color: "#3d3633"
                                
                                Label {
                                    anchors.centerIn: parent
                                    text: "🎮"
                                    font.pixelSize: 36
                                }
                            }
                            
                            Label {
                                text: "Xbox 360"
                                font.pixelSize: 18
                                font.weight: Font.Medium
                                color: colors.textPrimary
                                Layout.alignment: Qt.AlignHCenter
                            }
                            
                            Label {
                                text: "Coming Soon"
                                font.pixelSize: 12
                                color: colors.textTertiary
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }
                }
            }
        }
    }
    
    // RPCS3 Path Selection Page
    Component {
        id: usrdirPathPage
        
        Rectangle {
            width: stackView.width
            height: stackView.height
            color: colors.background
            
            ColumnLayout {
                anchors.centerIn: parent
                width: parent.width * 0.85
                spacing: 30
                
                Label {
                    text: "RPCS3 Setup"
                    font.pixelSize: 28
                    font.weight: Font.Bold
                    color: colors.textPrimary
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Label {
                    text: "Locate your Rock Band 3 USRDIR folder"
                    font.pixelSize: 16
                    color: colors.textSecondary
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Text.AlignHCenter
                }
                
                Label {
                    text: "Typically: ...\\RPCS3\\dev_hdd0\\game\\BLUS30463\\USRDIR"
                    font.pixelSize: 13
                    color: colors.textTertiary
                    Layout.alignment: Qt.AlignHCenter
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }
                
                // Path input card
                Card {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 10
                        
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 42
                            color: colors.inputBackground
                            radius: 8
                            border.color: pathTextField.activeFocus ? colors.primary : colors.cardBorder
                            border.width: 2
                            
                            TextInput {
                                id: pathTextField
                                anchors.fill: parent
                                anchors.margins: 12
                                text: controller.getUsrdirPath()
                                color: colors.textPrimary
                                verticalAlignment: TextInput.AlignVCenter
                                selectByMouse: true
                                clip: true
                                
                                Text {
                                    anchors.fill: parent
                                    text: "Enter path..."
                                    color: colors.textTertiary
                                    verticalAlignment: Text.AlignVCenter
                                    visible: !pathTextField.text && !pathTextField.activeFocus
                                }
                                
                                onTextChanged: {
                                    controller.setUsrdirPath(text)
                                }
                            }
                        }
                        
                        StyledButton {
                            text: "Browse..."
                            Layout.alignment: Qt.AlignRight
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 36
                            onClicked: controller.browseForUsrdirPath()
                        }
                    }
                }
                
                // Validation status
                Label {
                    text: controller.usrdirPathStatus
                    font.pixelSize: 14
                    color: controller.isUsrdirPathValid ? colors.success : colors.error
                    Layout.alignment: Qt.AlignHCenter
                    visible: text !== ""
                }
                
                Item { Layout.preferredHeight: 20 }
                
                StyledButton {
                    text: "Continue"
                    primary: true
                    Layout.alignment: Qt.AlignHCenter
                    enabled: controller.isUsrdirPathValid
                    onClicked: stackView.push(bridgePairingPage)
                }
                
                StyledButton {
                    text: "Back"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: stackView.pop()
                }
            }
        }
    }
    
    // Bridge Pairing Page
    Component {
        id: bridgePairingPage
        
        Rectangle {
            width: stackView.width
            height: stackView.height
            color: colors.background
            
            ColumnLayout {
                anchors.centerIn: parent
                width: parent.width * 0.85
                spacing: 30
                
                // Hue bridge icon
                Rectangle {
                    Layout.preferredWidth: 64
                    Layout.preferredHeight: 64
                    Layout.alignment: Qt.AlignHCenter
                    radius: 32
                    color: colors.cardBackground
                    
                    Label {
                        anchors.centerIn: parent
                        text: "🔌"
                        font.pixelSize: 32
                    }
                }
                
                Label {
                    text: "Connect to Hue Bridge"
                    font.pixelSize: 28
                    font.weight: Font.Bold
                    color: colors.textPrimary
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Label {
                    text: "We need to connect to your Hue Bridge\nto control your lights"
                    font.pixelSize: 16
                    color: colors.textSecondary
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Text.AlignHCenter
                }
                
                // Status display
                Card {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    
                    Label {
                        anchors.centerIn: parent
                        text: controller.bridgeStatus
                        font.pixelSize: 15
                        color: colors.textPrimary
                    }
                }
                
                StyledButton {
                    text: "Search for Bridge"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: controller.searchForBridge()
                }
                
                StyledButton {
                    text: "Connect"
                    primary: true
                    Layout.alignment: Qt.AlignHCenter
                    enabled: controller.bridgeFound
                    onClicked: controller.connectToBridge()
                }
                
                // Entertainment Area Selection
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 15
                    visible: controller.hasAvailableAreas
                    
                    // Update the model when this becomes visible
                    onVisibleChanged: {
                        if (visible) {
                            areaSelector.model = controller.getAvailableAreas()
                        }
                    }
                    
                    Label {
                        text: "Select Entertainment Area"
                        font.pixelSize: 16
                        color: colors.textPrimary
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    ComboBox {
                        id: areaSelector
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        model: []
                        
                        delegate: ItemDelegate {
                            width: areaSelector.width
                            contentItem: Text {
                                text: modelData
                                color: colors.textPrimary
                                font: areaSelector.font
                                verticalAlignment: Text.AlignVCenter
                            }
                            highlighted: areaSelector.highlightedIndex === index
                            background: Rectangle {
                                color: highlighted ? colors.cardBorder : colors.cardBackground
                            }
                        }
                        
                        onActivated: {
                            controller.selectArea(currentText)
                        }
                    }
                    
                    StyledButton {
                        text: "Continue"
                        primary: true
                        Layout.alignment: Qt.AlignHCenter
                        enabled: controller.currentArea !== ""
                        onClicked: stackView.push(mainPage)
                    }
                }
                
                StyledButton {
                    text: "Skip for Now"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: stackView.push(mainPage)
                }
            }
        }
    }
    
    // Main Page
    Component {
        id: mainPage
        
        Rectangle {
            width: stackView.width
            height: stackView.height
            color: colors.background
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 20
                
                // Header with connection status
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 15
                    
                    // Connection status badge
                    Rectangle {
                        Layout.preferredHeight: 36
                        Layout.fillWidth: true
                        radius: 18
                        color: controller.isStreaming() ? colors.successBg : colors.errorBg
                        border.color: controller.isStreaming() ? colors.successBorder : colors.errorBorder
                        border.width: 1
                        
                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 8
                            
                            Rectangle {
                                width: 8
                                height: 8
                                radius: 4
                                color: controller.isStreaming() ? colors.success : colors.error
                            }
                            
                            Label {
                                text: controller.isStreaming() ? "Streaming Active" : "Not Connected"
                                color: colors.textPrimary
                                font.pixelSize: 13
                            }
                        }
                    }
                    
                    // Settings button
                    Rectangle {
                        Layout.preferredWidth: 90
                        Layout.preferredHeight: 36
                        radius: 18
                        color: colors.accent
                        
                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 6
                            
                            Label {
                                text: "⚙️"
                                font.pixelSize: 16
                            }
                            
                            Label {
                                text: "Settings"
                                color: colors.textPrimary
                                font.pixelSize: 13
                                font.weight: Font.Medium
                            }
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: stackView.push(settingsPage)
                            onPressed: parent.opacity = 0.8
                            onReleased: parent.opacity = 1.0
                        }
                    }
                }
                
                // Entertainment Area Card
                Card {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    visible: controller.hasAvailableAreas
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 12
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Label {
                                text: "🎮"
                                font.pixelSize: 24
                            }
                            
                            Label {
                                text: controller.currentArea !== "" ? controller.currentArea : "No Area Selected"
                                font.pixelSize: 18
                                font.weight: Font.Bold
                                color: colors.textPrimary
                                Layout.fillWidth: true
                            }
                            
                            // Toggle placeholder
                            Rectangle {
                                width: 50
                                height: 28
                                radius: 14
                                color: colors.secondary
                                
                                Rectangle {
                                    width: 24
                                    height: 24
                                    radius: 12
                                    color: colors.textPrimary
                                    x: 2
                                    y: 2
                                }
                            }
                        }
                        
                            Label {
                                text: selectedPlatform + " • " + (controller.isStreaming() ? "Streaming" : "Idle")
                                font.pixelSize: 13
                                color: colors.textSecondary
                            }
                    }
                }
                
                // Current Effect Display
                Card {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 10
                        
                        Label {
                            text: "Current Effect"
                            font.pixelSize: 14
                            color: colors.textSecondary
                            Layout.alignment: Qt.AlignHCenter
                        }
                        
                        Label {
                            text: controller.currentEffect
                            font.pixelSize: 28
                            font.weight: Font.Bold
                            color: colors.textPrimary
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }
                
                Item { Layout.fillHeight: true }
                
                // Info message
                Label {
                    text: controller.isStreaming() ? "Lights are syncing automatically" : "Select an entertainment area to begin"
                    font.pixelSize: 14
                    color: colors.textSecondary
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
    
    // Settings Page
    Component {
        id: settingsPage
        
        Rectangle {
            width: stackView.width
            height: stackView.height
            color: colors.background
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 20
                
                // Header
                RowLayout {
                    Layout.fillWidth: true
                    
                    StyledButton {
                        text: "← Back"
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 40
                        onClicked: stackView.pop()
                    }
                    
                    Item { Layout.fillWidth: true }
                }
                
                Label {
                    text: "Settings"
                    font.pixelSize: 32
                    font.weight: Font.Bold
                    color: colors.textPrimary
                }
                
                // Settings sections
                Card {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        
                        Label {
                            text: "🔌"
                            font.pixelSize: 24
                        }
                        
                        Label {
                            text: "Reconfigure Hue Bridge"
                            font.pixelSize: 16
                            color: colors.textPrimary
                            Layout.fillWidth: true
                        }
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            controller.resetBridgePairing()
                            stackView.push(bridgePairingPage)
                        }
                        onPressed: parent.opacity = 0.8
                        onReleased: parent.opacity = 1.0
                    }
                }
                
                Card {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        
                        Label {
                            text: "🎮"
                            font.pixelSize: 24
                        }
                        
                        Label {
                            text: "Change Platform"
                            font.pixelSize: 16
                            color: colors.textPrimary
                            Layout.fillWidth: true
                        }
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: stackView.push(welcomePage)
                        onPressed: parent.opacity = 0.8
                        onReleased: parent.opacity = 1.0
                    }
                }
                
                Item { Layout.fillHeight: true }
            }
        }
    }
    
    // Connections to handle bridge pairing status changes
    Connections {
        target: controller
        
        function onBridgePaired() {
            stackView.push(mainPage)
        }
    }
}
