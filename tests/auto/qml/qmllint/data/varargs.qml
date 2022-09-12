import QtQml
import TestTypes

MyComponent {
    id: self
    Component.onCompleted: createObject({a : 12}, self, 1, 2, 3)
}
