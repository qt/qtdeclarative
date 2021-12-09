import QtQml 2.0
Cycle2 {
    id: itt
    QtObject {
        property var thing: itt
    }
}
