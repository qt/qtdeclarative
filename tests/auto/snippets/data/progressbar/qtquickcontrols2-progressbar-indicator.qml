import QtQuick 2.0
import QtQuick.Controls 2.0

ProgressBar {
    value: 0.5
    Rectangle {
        parent: indicator
        width: indicator.childrenRect.width
        height: indicator.height
        color: "transparent"
        border.color: "red"
    }
}
