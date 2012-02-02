import QtQuick 1.1
import Qt.labs.gestures 1.0
import Flashback 1.0

Rectangle {
    id: appWindow
    width: 256; height: 224
    focus: true

    property int room;

    Keys.onPressed: {
        switch (event.key) {
        case Qt.Key_Left:  goto_room(Level.LEFT);  break
        case Qt.Key_Right: goto_room(Level.RIGHT); break
        case Qt.Key_Up:    goto_room(Level.UP);    break
        case Qt.Key_Down:  goto_room(Level.DOWN);  break
        default:
            return
        }
        event.accepted = true;
    }

    Component.onCompleted: {
        level.load(2)
        room = 1
        img.source = "image://level/2/" + room
    }

    Image {
        id: img
    }

//    Cutscene {
//        id: cutscene
//        anchors.fill: parent
//        Component.onCompleted: { cutscene.playing = true }
//    }

//    Label {
//        anchors.centerIn: parent
//        text: "paused"
//        visible: !cutscene.playing
//    }

//    MouseArea {
//        anchors.fill: parent
//        onClicked: { cutscene.playing = !cutscene.playing }
//    }

//    GestureArea {
//        anchors.fill: parent
//        onSwipe: {
//            console.log(swipeAngle)
//        }
//        onTap: { cutscene.playing = !cutscene.playing }
//    }

    Level {
        id: level
    }


    function goto_room(dir)
    {
        var new_room = level.adjacentRoomAt(room, dir)
        if (new_room < 0)
            return
        room = new_room
        img.source = "image://level/2/" + room
    }
}
