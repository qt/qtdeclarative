import QtQuick 2.0
Item {
    property int value: 42
    anchors.topMargin: value // binding on anchors (private property)
}
