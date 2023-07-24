import QtQml
import TestTypes

QtObject {
    readonly property ObjectWithStringListMethod foo: ObjectFactory.getFoo()
    Component.onCompleted: console.log(foo ? foo.names().length : "-")
}
