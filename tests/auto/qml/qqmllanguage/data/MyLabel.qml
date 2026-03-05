import QtQml

QtObject {
    property alias label: labelText
    property Binding l: Binding {
        id: labelText
    }
}
