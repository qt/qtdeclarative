import QtQuick

Text {
    id: root
    anchors.alignWhenCentered: false
    anchors.topMargin: 1
    anchors.bottomMargin: Math.max(32, 31) + 10 // == 42

    font.family: "Helvetica"
    font.pointSize: 4
    font.letterSpacing: Math.max(2, 3)

    Text {
        anchors.topMargin: root.anchors.topMargin
        anchors.bottomMargin: root.anchors.bottomMargin
    }
}
