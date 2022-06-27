pragma Strict
import QtQml

QtObject {
    id: self
    signal foo()
    objectName: self.foo() + 1
}
