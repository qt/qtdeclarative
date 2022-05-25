import QtQuick
import QmltcTests 1.0
TypeWithDeferredProperty {
    id: root
    property int width: 42

    deferredProperty: Rectangle {
        width: root.width * 2
        implicitHeight: 4
        height: implicitHeight
        Rectangle {
            width: root.width // + parent.width
            height: parent.height
        }
    }
}
