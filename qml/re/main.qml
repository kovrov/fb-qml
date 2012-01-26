import QtQuick 1.1
import Flashback 1.0

Rectangle {
    id: appWindow
    width: 240; height: 128

    Cutscene {
        id: cutscene
        anchors.fill: parent
        Component.onCompleted: { cutscene.playing = true }
    }

    Label {
        anchors.centerIn: parent
        text: "paused"
        visible: !cutscene.playing
    }

    MouseArea {
        anchors.fill: parent
        onClicked: { cutscene.playing = !cutscene.playing }
    }

}
