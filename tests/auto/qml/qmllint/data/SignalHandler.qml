import QtQuick 2.0

MouseArea {
    onDoubleClicked: {
        console.log(mouse);
        // do further things
    }
    onClicked: console.info(mouse)
    onPositionChanged: {
        console.log(mouse)
    }
    onPressAndHold: console.warn(mouse)
}
