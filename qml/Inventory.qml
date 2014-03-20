import QtQuick 2.2
import reminiscence 1.0


FocusScope {
    property alias model: list.model
    property alias currentIndex: list.currentIndex

    implicitHeight: backdrop.height
    implicitWidth: backdrop.width

    Keys.onUpPressed: {
        if (list.canGoUp)
            list.currentIndex = Math.min(list.currentIndex + 4, list.count - 1)
    }

    Keys.onDownPressed: {
        if (list.canGoDown)
            list.currentIndex = Math.max(list.currentIndex - 4, 0)
    }

    Grid {
        id: backdrop
        anchors.centerIn: parent
        columns: 9
        Repeater {
            model: 45
            delegate: Image {
                smooth: false
                source: "image://icon/global/" + (index + 31)
            }
        }
    }

    Item {
        anchors { centerIn: parent; verticalCenterOffset: -15 }
        width: 16 * 7
        height: 16
        clip: true

        GridView {
            id: list
            property bool canGoUp: list.count - list.count % 4 > list.currentIndex
            property bool canGoDown: list.currentIndex + 1 > 4
            anchors { centerIn: parent }
            width: 16 * 8
            height: 16 * 2
            cellWidth : 16 * 2
            cellHeight : 16 * 2
            focus: true
            verticalLayoutDirection: GridView.BottomToTop
            delegate: Item {
                id: delegate
                width : 16*2
                height : 16*2
                Image {
                    anchors.centerIn: parent
                    source: "image://icon/global/" + modelData
                    smooth: false
                }
                Image {
                    anchors.centerIn: parent
                    source: "image://icon/global/76"
                    smooth: false
                    visible: delegate.GridView.isCurrentItem
                }
            }
        }
    }

    Column {
        anchors { centerIn: parent; verticalCenterOffset: -12 }
        spacing: 17
        Image {
            source: "image://icon/global/77"
            smooth: false
            opacity: list.canGoUp ? 1 : 0
        }
        Image {
            source: "image://icon/global/78"
            smooth: false
            opacity: list.canGoDown ? 1 : 0
        }
    }
}
