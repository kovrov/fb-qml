import QtQuick 2.2

Rectangle {
    signal startSelected()
    signal quitSelected()

    color: "black"

    Image {
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
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
