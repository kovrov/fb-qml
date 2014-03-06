import QtQuick 2.2
import QtQuick.Window 2.1
import reminiscence 1.0


Window {
    width: 256*3; height: 224*3
    title: "Flashback"

    Item {
        id: app

        Component.onCompleted: { state = "intro" }
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
    }

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
                app.state = "game"

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
                    app.state = "menu"
                }
            }
        }
    }

    Cutscene {
        id: cutscene
        visible: false
        anchors.fill: parent

        property var queue: []

        onFinished: {
            if (queue.length === 0) {
                app.state = "menu"
                return
            }
            var tmp = queue  // workaround qml properties limitations
            play(tmp.shift())
            queue = tmp
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                app.state = "menu"
            }
        }
    }
}
