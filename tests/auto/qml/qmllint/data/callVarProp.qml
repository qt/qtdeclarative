import QtQml

QtObject {
    property var foo: () => {}

    Component.onCompleted: foo()
}
