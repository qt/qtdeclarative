import QtQuick
import QtQuick.Window

Window {
    x: 0
    y: 0
    property string screenName: screen.name
    property string attachedScreenName: Screen.name

    Component.onCompleted: {
        x = screen.width + 1
    }
}
