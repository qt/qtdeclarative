import QtQml
import Invisible

QtObject {
    id: root
    property list<InvisibleListElement> customList

    property QtObject a: QtObject {
        property var x: root.customList
    }
}
