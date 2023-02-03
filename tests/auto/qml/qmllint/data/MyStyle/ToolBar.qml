import QtQuick
import QtQuick.Templates as T
import MyStyle

T.ToolBar {
    id: control

    property color c: MyStyle.toolBarColor
    background: Rectangle {
        color: MyStyle.toolBarColor
    }
}
