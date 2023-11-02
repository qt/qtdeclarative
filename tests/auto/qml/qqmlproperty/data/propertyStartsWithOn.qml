import QtQml

QtObject {
    id: root
    property int onlineStatus
    property Binding b: Binding {
        root.onlineStatus: 12
    }
}
