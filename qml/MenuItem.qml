import QtQuick 2.2

Text {
    signal clicked()
    color: "yellow"
    font.pointSize: 16
    font.bold: true
    style: Text.Raised
    styleColor: "#80000000"
    MouseArea {
        anchors.fill: parent
        onClicked: parent.clicked()
    }
}
