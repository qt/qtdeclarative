import QtQml 2.0
QtObject {
    id: root
    property int foo: 42
    property bool success: false
    Component.onCompleted: {
        success = true; // break here
    }
}
