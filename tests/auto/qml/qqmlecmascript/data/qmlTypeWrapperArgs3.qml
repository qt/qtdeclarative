import QtQml
import Qt.test.singletonWithEnum

QtObject {
    id: root
    required property QtObject invokableObject

    Component.onCompleted: {
        root.invokableObject.method_typeWrapper(Component)
        root.invokableObject.method_typeWrapper(SingletonWithEnum)
    }
}
