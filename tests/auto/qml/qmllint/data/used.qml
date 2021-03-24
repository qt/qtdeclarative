import QtQml as Qml
import QtQuick as Quick
import QtQuick 2.0 as QuickLegacy
import QtQuick

Item {
    property var bar: Qml.QtObject {}
    Item {
        property var foo: Quick.Screen.pixelDensity
        property var foo2: parent.QuickLegacy.Screen.pixelDensity
    }
}
