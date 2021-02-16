import QtQml 2.0

QtObject {
    property string hello: "Hello from parent"
    property QtObject origin
    default property alias child: origin

    QtObject {
        property string hello: "Hello from parent.child (alias)"
    }
}
