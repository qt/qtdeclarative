import QtQuick 2.0

ListView {
    id: root
    width: 300; height: 300
    orientation: ListView.Horizontal
    highlightRangeMode: ListView.StrictlyEnforceRange
    model: 2
    delegate: Text {
        verticalAlignment: Qt.AlignVCenter; horizontalAlignment: Qt.AlignHCenter
        width: root.width; height: root.height
        text: "page " + index
    }
}
