import QtQuick

Item {
    height: undefined
    implicitHeight: 30
    property int steps: 0

    Behavior on height {
        NumberAnimation {
            duration: 100
        }
    }

    onHeightChanged: ++steps

    Component.onCompleted: {
        height = Qt.binding(() => implicitHeight);
        implicitHeight = 60;
    }
}
