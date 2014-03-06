import QtQuick 2.2


ListView {
    signal itemSelected(int index)
    model:
        ListModel {
        ListElement { display: "Titan / The Jungle" }
        ListElement { display: "Titan / New Washington" }
        ListElement { display: "Titan / Death Tower Show" }
        ListElement { display: "Earth / Surface" }
        ListElement { display: "Earth / Paradise Club" }
        ListElement { display: "Planet Morphs / Surface" }
        ListElement { display: "Planet Morphs / Inner Core" }
    }
    delegate: Rectangle {
        color: "#60000000"
        height: label.height + 24
        width: parent.width
        Text {
            id: label
            x: 16
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            font.pointSize: 24
            text: display
        }
        MouseArea {
            anchors.fill: parent
            onClicked: itemSelected(model.index)
        }
    }
}
