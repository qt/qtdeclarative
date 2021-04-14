import QtQml as Qml
import QtQuick as Quick
import QtQuick 2.0 as QuickLegacy
import QtQuick
import Things

Item {
    property var bar: Qml.QtObject {}
    Item {
        property var foo: Quick.Screen.pixelDensity
        property var foo2: parent.QuickLegacy.Screen.pixelDensity
    }
    required property Something moo
    property bool boolVar: false
    property real realVar: 17.17
    property double doubleVar: 18.18
}
