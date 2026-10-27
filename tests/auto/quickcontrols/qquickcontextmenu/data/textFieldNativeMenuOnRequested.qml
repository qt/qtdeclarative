import QtQuick.Controls

Pane {
    id: root
    width: 400
    height: 400

    property bool menuOpened: false

    TextField {
        objectName: "textField"
        text: "123,456"
        width: parent.width

        ContextMenu.onRequested: {
            const menu = ContextMenu.menu
            menu.popupType = Popup.Native
            menu.aboutToShow.connect(() => {
                root.menuOpened = true
                Qt.callLater(menu.close)
            })
        }
    }
}
