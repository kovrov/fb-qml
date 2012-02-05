import QtQuick 1.1
import Flashback 1.0

Rectangle {
    id: appWindow
    width: 256; height: 224
    color: "#000000"

    MapBrowser {
        id: map
        anchors.fill: parent
    }

//    Cutscene {
//        id: cutscene
//        anchors.fill: parent
//        Component.onCompleted: { playing = true }
//    }

//    Label {
//        anchors.centerIn: parent
//        text: "paused"
//        visible: !cutscene.playing
//    }

//    MouseArea {
//        anchors.fill: parent
//        onClicked: {
//            cutscene.playing = !cutscene.playing
//        }
//    }
}
