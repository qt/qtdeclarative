import QtQuick
import test

TestBindable {
    property int targetValue: 0
    prop: targetValue
    property alias enableBehavior: behavior.enabled
    Behavior on prop {
        id: behavior
        NumberAnimation {duration: 100}
    }
}
