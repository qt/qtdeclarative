import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 100
    height: 100
    property alias menu: menu
    Menu {
        id: menu
        MenuItem{
            text: "<img />"
        }
    }
}
