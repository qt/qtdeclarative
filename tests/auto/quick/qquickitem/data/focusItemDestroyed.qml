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
                timer.running = true
            }
            Timer {
                id: timer
                interval: 100
                onTriggered: {
                    dlg.close()
                    loader.active = false
                }
            }
        }
    }
}
