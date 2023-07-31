import QtQuick

Item {
    property var val: undefined
    property var observes: width
    width: val
    implicitWidth: 200
}
