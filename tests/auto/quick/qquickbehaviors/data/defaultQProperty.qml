import QtQuick

Item {
    property int heightChanges: 0
    property int widthChanges: 0
    implicitWidth: 50
    implicitHeight: 50
    height: { return 0 }
    width: { return 10 }
    Behavior on height { NumberAnimation { duration: 300 } }
    Behavior on width { NumberAnimation { duration: 300 } }
    onHeightChanged: ++heightChanges
    onWidthChanged: ++widthChanges
}
