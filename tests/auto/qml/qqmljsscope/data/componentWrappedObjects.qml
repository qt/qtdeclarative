import QtQuick
import Qt.labs.qmlmodels
Item {
    // Note: use objectName instead of ids to uniquely identify types
    property Component nonWrapped1: Component { objectName: "nonWrapped1"; Rectangle {} }
    property Component nonWrapped2: ComponentType { objectName: "nonWrapped2" }
    property Component nonWrapped3: DelegateChooser { objectName: "nonWrapped3" }

    property Component wrapped: Text { objectName: "wrapped" }

    component MyInlineComponent: Item {
        property Component wrapped: Text { objectName: "wrapped" }

        TableView {
            delegate: Text { objectName: "wrapped2" }
        }

        TableView {
            delegate: ComponentType { objectName: "wrapped3" }
        }
    }

    property var wrappedInInlineComponent: MyInlineComponent {}
}
