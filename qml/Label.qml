import QtQuick 2.2

Rectangle {
    property alias text: label.text
    width: label.width + 24; height: label.height + 24; radius: 8
    color: "#60000000"
    Text {
        id: label
        anchors.centerIn: parent
        color: "white"
        font.pointSize: 24
    }
}
