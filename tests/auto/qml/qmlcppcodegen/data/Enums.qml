import QtQml
import QtQuick.Layouts

QtObject {
    id: root
    property int appState: Enums.AppState.Blue
    property string color: "blue"

    enum AppState {
        Red,
        Green,
        Blue
    }

    onAppStateChanged: {
        if (appState === Enums.AppState.Green)
            root.color = "green"
        else if (appState === Enums.AppState.Red)
            root.color = "red"
    }

    property Timer timer: Timer {
        onTriggered: root.appState = Enums.AppState.Green
        running: true
        interval: 100
    }

    Layout.alignment: Qt.AlignCenter
}
