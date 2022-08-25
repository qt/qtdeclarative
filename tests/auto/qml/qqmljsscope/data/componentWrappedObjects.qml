import QtQuick
import Qt.labs.qmlmodels
Item {
    // Note: use properties instead of ids to uniquely identify types
    property Component nonWrapped1: Component { property int nonWrapped1; Rectangle {} }
    property Component nonWrapped2: ComponentType { property int nonWrapped2 }
    property Component nonWrapped3: DelegateChooser { property int nonWrapped3 }

    property Component wrapped: Text { property int wrapped }

    component MyInlineComponent: Item {
        property Component wrapped: Text { property int wrapped }

        TableView {
            delegate: Text { property int wrapped2 }
        }

        TableView {
            delegate: ComponentType { property int wrapped3 }
        }
    }

    property var wrappedInInlineComponent: MyInlineComponent {}
}
