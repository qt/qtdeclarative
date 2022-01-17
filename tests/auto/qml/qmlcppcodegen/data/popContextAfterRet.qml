pragma Strict
import QtQml

QtObject {
    id: root
    property int stackViewDepth: 0
    onStackViewDepthChanged:  {
        if (stackViewDepth > 1) {
            root.objectName = "backgroundBlur"
            return true
        }
        root.objectName = "backgroundImage"
        return false
    }
}
