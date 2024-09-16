import QtQuick
import QtQuick.Window

Window {
    id: root
    function handleKey(e: KeyEvent) {}
    function handleMouse(e: MouseEvent) {}
    function handleWheel(e: WheelEvent) {}
    function handleClose(e: CloseEvent) {}

    Item {
        Keys.onDeletePressed: root.handleKey
        MouseArea {
            onClicked: root.handleMouse
            onWheel: root.handleWheel
        }
    }

    onClosing: handleClose
}
