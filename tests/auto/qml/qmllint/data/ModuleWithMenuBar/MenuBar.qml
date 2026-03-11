import QtQuick

Rectangle {
    readonly property int currentIndex: lv.currentIndex
    readonly property int custWidth: lv.highlightMoveDuration
    anchors.fill: parent
    color: "red"
    ListView {
        id: lv
        highlightMoveDuration: 500
    }
}
