import QtQuick

Item {
    width: ListView.view.width+40
    height: ListView.view.height
    Component.onCompleted: ListView.view.height = 10
}
