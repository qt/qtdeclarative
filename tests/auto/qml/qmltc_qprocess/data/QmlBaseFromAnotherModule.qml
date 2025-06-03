import QtQml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window

Item {
    property ScrollBar myBar: ScrollBar {}
    function f(a: ScrollBar): ScrollBar {}

    // C++ defined QML types from other modules are fine
    property Item myItem: Item {}
    function g(a: Item): Item {}
}
