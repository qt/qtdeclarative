import QtQml 2.0
import QtQuick 2.0

QtObject {
    property string hello


    Component.onCompleted: {
        hello = "Hello, World!";
    }

    signal sDestroying()
    Component.onDestruction: {
        sDestroying();
    }
}
