import QtQml

QtObject {
    id: root
    required property QtObject invokableObject

    Component.onCompleted: root.invokableObject.method_QObject(Component)
}
