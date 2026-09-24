import QtQuick

Item {
    id: root
    width: 320
    height: 240

    // Set to true once the deactivated Loader subtree has been deleted, i.e.
    // after ~QQuickItemView has run.
    property bool viewDestroyed: false

    Loader {
        id: loader
        anchors.fill: parent

        sourceComponent: ListView {
            model: 40
            delegate: Item { height: 24 }
            Component.onDestruction: root.viewDestroyed = true
        }
    }

    Component.onCompleted: {
        const view = loader.item;
        loader.active = false;
        view.contentY = 1000;
    }
}
