import QtQml

QtObject {
    Component.onDestruction: {
        console.log("evil!");
    }

    Component.onCompleted: {
        Component.onDestruction()
    }
}
