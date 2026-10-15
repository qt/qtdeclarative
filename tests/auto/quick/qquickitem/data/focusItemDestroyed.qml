import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: top
    width: 640
    height: 480
    visible: true

    Loader {
        id: loader
        sourceComponent: Dialog {
            id: dlg
            onActiveFocusChanged: {
                if(!activeFocus)
                    forceActiveFocus()
            }
            Component.onCompleted: {
                dlg.open()
            }
            Timer {
                id: timer
                objectName: "timer"
                interval: 100
                onTriggered: {
                    dlg.close()
                    loader.active = false
                }
            }
        }
    }
}
