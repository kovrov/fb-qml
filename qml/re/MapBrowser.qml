import QtQuick 1.1
import Flashback 1.0


Item {
    property int levelId: -1
    property int room: -1


    onLevelIdChanged:  {
        console.log("onLevelIdChanged: " + levelId)
        level.load(levelId)
        room = level.initialRoom
        img.source = "image://level/" + levelId + "/" + room
    }


    Image {
        width: parent.width
        height: parent.height
        fillMode: Image.PreserveAspectFit
        id: next
        anchors.centerIn: img
        visible: false
    }

    Image {
        width: parent.width
        height: parent.height
        fillMode: Image.PreserveAspectFit
        id: img
        property bool normal: true
        onNormalChanged: {
            if (normal) {
                next.anchors.verticalCenterOffset = next.anchors.horizontalCenterOffset = 0
                next.x = next.y = 0
                next.visible = false
            }
            else
                next.visible = true
        }
    }

    Rectangle {
        width: (img.width - img.paintedWidth) / 2
        height: parent.height
        color: "black"
    }

    Rectangle {
        anchors.right: parent.right
        width: (img.width - img.paintedWidth) / 2
        height: parent.height
        color: "black"
    }

    Level {
        id: level
    }

    MouseArea {
        property variant origin // point
        property int value
        property int swipeDirection: -1
        anchors.fill: parent

        onPressed: {
            state = "unrecognized"
            origin = Qt.point(mouse.x, mouse.y)
        }

        onReleased: {
            if (state == "ready") {
                var img_source = img.source
                img.source = next.source
                next.source = img_source
                switch (swipeDirection) {
                case Level.RIGHT: {
                    img.x += img.paintedWidth
                    next.anchors.horizontalCenterOffset = -img.paintedWidth
                  } break
                case Level.LEFT: {
                    img.x -= img.paintedWidth
                    next.anchors.horizontalCenterOffset = +img.paintedWidth
                  } break
                case Level.DOWN: {
                    img.y += img.paintedHeight
                    next.anchors.verticalCenterOffset = -img.paintedHeight
                  } break
                case Level.UP: {
                    img.y -= img.paintedHeight
                    next.anchors.verticalCenterOffset = +img.paintedHeight
                  } break
                }
                room = level.adjacentRoomAt(room, swipeDirection)
            }
            value = 0
            state = ""
        }

        onMousePositionChanged: {
            switch (state) {
            case "ready":
            case "cancelled":
                updatePositions(mouse.x, mouse.y)
                break
            case "unrecognized":
                recognize(mouse.x, mouse.y)
                break
            }
        }

        function updatePositions(x, y) {
            var h = x - origin.x
            var v = y - origin.y
            switch (swipeDirection) {
            case Level.RIGHT: {
                if (h < value)
                    value = h
                else if (h > value + 16)
                    state = "cancelled"
                img.x = h
              } break
            case Level.LEFT: {
                if (h > value)
                    value = h
                else if (h < value - 16)
                    state = "cancelled"
                img.x = h
              } break
            case Level.DOWN: {
                if (v < value)
                    value = v
                else if (v > value + 16)
                    state = "cancelled"
                img.y = v
              } break
            case Level.UP: {
                if (v > value)
                    value = v
                else if (v < value - 16)
                    state = "cancelled"
                img.y = v
              } break
            }
        }

        function recognize(x, y) {
            var h = x - origin.x
            var v = y - origin.y
            if (h > 32) {
                if (-1 == level.adjacentRoomAt(room, Level.LEFT))
                    return
                swipeDirection = Level.LEFT
                next.anchors.horizontalCenterOffset = -img.paintedWidth
            }
            else if (h < -32) {
                if (-1 == level.adjacentRoomAt(room, Level.RIGHT))
                    return
                swipeDirection = Level.RIGHT
                next.anchors.horizontalCenterOffset = img.paintedWidth
            }
            else if (v > 32) {
                if (-1 == level.adjacentRoomAt(room, Level.UP))
                    return
                swipeDirection = Level.UP
                next.anchors.verticalCenterOffset = -img.paintedHeight
            }
            else if (v < -32) {
                if (-1 == level.adjacentRoomAt(room, Level.DOWN))
                    return
                swipeDirection = Level.DOWN
                next.anchors.verticalCenterOffset = img.paintedHeight
            }
            else
                return
            origin = Qt.point(x, y)
            var n = level.adjacentRoomAt(room, swipeDirection)
            next.source = "image://level/" + levelId + "/" + n
            state = "ready"
        }

        states: [
            State { name: ""; PropertyChanges { target:img; x:0; y:0; normal:true }},
            State { name: "unrecognized"; },
            State { name: "ready";
                PropertyChanges {
                    target: img;
                    normal: false;
                    restoreEntryValues: false
                } },
            State { name: "cancelled"; }
        ]

        transitions: [
            Transition {
                from: "cancelled"; to: "";
                SequentialAnimation {
                     NumberAnimation { target:img; properties:"x,y"; duration:100 }
                     PropertyAction { target:img; properties:"normal" }
                 }
            },
            Transition {
                from: "ready"; to: ""
                SequentialAnimation {
                     NumberAnimation { target:img; properties:"x,y"; duration:100 }
                     PropertyAction { target:img; properties:"normal" }
                 }
            }
        ]
    }
}
