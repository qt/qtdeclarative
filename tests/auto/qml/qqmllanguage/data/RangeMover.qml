import QtQuick

Item {
    readonly property alias rangeWidth: selectedRange.width
    Rectangle {
        id: selectedRange
        width: parent.height
    }
}
