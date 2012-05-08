import QtQuick 1.1
import Flashback 1.0

Rectangle {
    id: appWindow
    width: 854; height: 480
    color: "#000000"

    Menu {
        id: menu
        visible: false
        anchors.fill: parent
        onStartSelected: levelMenu.visible = true
        onQuitSelected: Qt.quit()

        LevelMenu {
            id: levelMenu
            anchors.fill: parent
            visible: false
            onItemSelected: {
                visible = false
                map.levelId = index
                appWindow.state = "game"

            }
        }
    }

    MapBrowser {
        id: map
        visible: false
        anchors.fill: parent
        Label {
            text: "[~]"
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    appWindow.state = "menu"
                }
            }
        }
    }

    Cutscene {
        id: cutscene
        visible: false
        anchors.fill: parent

        property variant queue: []
        MouseArea {
            anchors.fill: parent
            onClicked: {
                appWindow.state = "menu"
            }
        }
        onFinished: {
            if (queue.length == 0) {
                appWindow.state = "menu"
                return
            }
            var tmp = queue  // workaround qml properties limitations
            play(tmp.shift())
            queue = tmp
        }
    }

    states: [
        State {
            name: "intro"
            StateChangeScript {
                script: {
                    menu.visible = false
                    map.visible = false
                    cutscene.visible = true
                    cutscene.play("logos")
                    cutscene.queue = ["intro1", "intro2"]
                }
            }
        },
        State {
            name: "menu"
            PropertyChanges { restoreEntryValues: false; target: menu; visible: true }
            PropertyChanges { restoreEntryValues: false; target: map; visible: false }
            PropertyChanges { restoreEntryValues: false; target: cutscene; visible: false }
            StateChangeScript { script: cutscene.stop() }
        },
        State {
            name: "game"
            PropertyChanges { restoreEntryValues: false; target: menu; visible: false }
            PropertyChanges { restoreEntryValues: false; target: map; visible: true }
            PropertyChanges { restoreEntryValues: false; target: cutscene; visible: false }
            StateChangeScript { script: cutscene.stop() }
        }
    ]

    Component.onCompleted: { state = "intro" }
}
