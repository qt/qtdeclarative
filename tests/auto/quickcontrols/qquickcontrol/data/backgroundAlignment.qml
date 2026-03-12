import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T

ApplicationWindow {
    width: control.width
    height: control.height
    visible: true
    color: "white"
    T.Button {
        id: control
        implicitWidth: implicitBackgroundWidth + leftInset + rightInset
        implicitHeight: implicitBackgroundHeight + topInset + bottomInset
        topInset: 6
        bottomInset: 6
        background: Rectangle {
            implicitWidth: 200
            implicitHeight: 50
            color: "red"
            layer.enabled: true
        }
    }
}

