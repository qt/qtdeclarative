import QtQml

QtObject {
    property alias strokeStyle: path.restoreMode
    property Binding p: Binding { id: path }
}
