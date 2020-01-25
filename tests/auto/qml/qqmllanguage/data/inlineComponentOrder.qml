import QtQuick 2.15

Item {
    width: 600
    height: 480
    IC2 {
        objectName: "icInstance"
        anchors.centerIn: parent
    }

    component IC2: IC1 {}
    component IC0: Rectangle {
        height: 200
        width: 200
        color: "blue"
    }
    component IC1: IC0 {}


}
