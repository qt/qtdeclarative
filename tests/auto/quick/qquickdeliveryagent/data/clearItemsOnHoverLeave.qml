import QtQuick

Rectangle{
    id: mainWindow

    visible: true
    width: 800
    height: 600

    Column {
        anchors.fill: parent
        MouseArea {
            width: parent.width
            height: parent.height/3
            hoverEnabled: true
        }
        MouseArea {
            id: mouseArea
            width: parent.width
            height: parent.height/3
            hoverEnabled: true
            onExited: {
                Window.window.close();
            }
        }
        MouseArea {
            width: parent.width
            height: parent.height / 3
            hoverEnabled: true
        }
    }
}
