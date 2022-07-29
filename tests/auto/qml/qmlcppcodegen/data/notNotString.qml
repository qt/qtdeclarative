pragma Strict
import QML

QtObject {
    id: self
    property bool notNotString: !!self.objectName
}
