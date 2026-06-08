import QtQuick 2.0
import QtQuick.Window 6.13 as Window

Item {
    width: 100
    height: 100
    property int w: Window.Screen.width
    property int h: Window.Screen.height
    property int curOrientation: Window.Screen.orientation
    property int priOrientation: Window.Screen.primaryOrientation
    property real devicePixelRatio: Window.Screen.devicePixelRatio
    property int vx: Window.Screen.virtualX
    property int vy: Window.Screen.virtualY
    property rect screenGeometry: Window.Screen.geometry
    property rect screenAvailableGeometry: Window.Screen.availableGeometry
    property rect desktopGeometry: Window.Screen.desktopGeometry
    property rect availableDesktopGeometry: Window.Screen.availableDesktopGeometry

    property int screenCount: Qt.application.screens.length

    property variant allScreens
    Component.onCompleted: {
        allScreens = [];
        var s = Qt.application.screens;
        for (var i = 0; i < s.length; ++i)
            allScreens.push(s[i]);
    }
}
