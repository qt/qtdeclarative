import QtQuick 2.0

Item {
    id: root
    property int oldi: 0
    property int i: 0

    Loader {
         id: loader
         objectName: "loader"
         active: true
    }

    Component.onCompleted: {
         loader.setSource("CacheClearTest.qml", {i: 12})
         root.oldi = loader.item.i
         loader.setSource("CacheClearTest.qml")
         root.i = loader.item.i // should be 42
    }
}
