import QtQuick 1.1

Rectangle {
    signal startSelected()
    signal quitSelected()

    color: "black"

    Image {
        source: "image://menu/menu1"
    }

    Column {
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 32
        spacing: 16
        MenuItem {
            text: qsTr("START")
            onClicked: parent.parent.startSelected()
        }
        MenuItem {
            text: qsTr("QUIT")
            onClicked: parent.parent.quitSelected()
        }
    }
}
