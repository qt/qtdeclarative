import QtQuick 2.11

Item {
    visible: true
    width: 320
    height: 200
    property int val: other.val

    Rectangle {
        id: other
        anchors.fill: parent;
        property int val: undefined / 2
    }
}
