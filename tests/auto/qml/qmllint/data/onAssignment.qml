import QtQuick

Item {
    property bool pressed: false
    SequentialAnimation on pressed {}
    property int wrong: pressed.loops
}
