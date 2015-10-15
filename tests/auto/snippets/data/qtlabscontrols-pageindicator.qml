import QtQuick 2.0
import Qt.labs.controls 1.0

Rectangle {
    width: 100
    height: 100
    border.color: Theme.frameColor

    //! [1]
    PageIndicator {
        count: 5
        currentIndex: 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
    }
    //! [1]
}
