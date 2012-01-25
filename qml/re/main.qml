import QtQuick 1.1
import Flashback 1.0

Rectangle {
    id: appWindow
    width: 240; height: 128

    Cutscene {
        id: cutscene
        anchors.fill: parent
    }

    MouseArea {
        anchors.fill: parent
        onClicked: cutscene.play()
    }
}
