import QtQuick
import QtQuick.Controls.Basic

pragma ComponentBehavior: Bound

Item {
    Component {
        id: paneComp
        Pane {
            Item {
                id: stack
                function hhh() {
                }
            }
            Component.onDestruction: stack.hhh()
        }
    }

    Component {
        id: loaderComp
        Loader {
            sourceComponent: paneComp
        }
    }

    Component.onCompleted: {
        loaderComp.createObject(null, {})
        loaderComp.createObject(null, {})
        loaderComp.createObject(null, {})
        gc()
    }
}
