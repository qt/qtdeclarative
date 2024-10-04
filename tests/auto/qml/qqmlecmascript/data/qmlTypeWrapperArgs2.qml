import QtQml
import QtQml as NS

QtObject {
    id: root
    required property QtObject invokableObject

    Component.onCompleted: root.invokableObject.method_QObject(NS)
}
