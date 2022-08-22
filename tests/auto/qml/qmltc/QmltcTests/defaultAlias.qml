import QtQml

DefaultPropertyAliasChild {
    id: self
    property string hello: "Hello from parent"

    QtObject {
        property string hello: "Hello from parent.child (alias)"
    }
}
