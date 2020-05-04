import QtQuick 2.0

Item {
    id: root
    property int i: 0

    Loader {
        id: loader
        objectName: "loader"
        active: false
    }

    Component.onCompleted: {
        loader.setSource("CacheClearTest.qml", {i: 12})
        loader.active = true
        loader.active = false
        loader.active = true
        root.i = loader.item.i // should be 12
    }
}
