import QtQml

QtObject {
    property Enums app: Enums {
        appState: 0
    }

    property string color: app.color
}
