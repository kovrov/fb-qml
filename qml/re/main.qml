import QtQuick 1.1
import Flashback 1.0

Rectangle {
    id: appWindow
    width: 256; height: 224
    color: "#000000"

    Menu {
        id: menu
        visible: false
        anchors.fill: parent
        onStartSelected: appWindow.state = "game"
        onOptionsSelected: console.log("TODO: options")
        onQuitSelected: Qt.quit()
    }

    MapBrowser {
        id: map
        visible: false
        anchors.fill: parent
    }

    Cutscene {
        id: cutscene
        visible: false
        anchors.fill: parent

        Label {
            anchors.centerIn: parent
            text: "paused"
            visible: (!cutscene.playing)
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                appWindow.state = "menu"
            }
        }
    }

    states: [
        State {
            name: "intro"
            PropertyChanges { target: menu; visible: false }
            PropertyChanges { target: map; visible: false }
            PropertyChanges { target: cutscene; visible: true; playing: true }
        },
        State {
            name: "menu"
            PropertyChanges { target: menu; visible: true }
            PropertyChanges { target: map; visible: false }
            PropertyChanges { target: cutscene; visible: false; playing: false }
        },
        State {
            name: "game"
            PropertyChanges { target: menu; visible: false }
            PropertyChanges { target: map; visible: true; levelId: 1 }
            PropertyChanges { target: cutscene; visible: false; playing: false }
        }
    ]

    Component.onCompleted: { state = "intro" }
}
