import QtQml

QtObject {
    id: root
    property alias someProperty: internal.someProperty

    property Timer t: Timer {
        interval: 1
        running: true
        onTriggered: root.someProperty = null
    }

    property QtObject a: QtObject {
        id: someObjectInstance
    }

    property QtObject b: QtObject {
        id: internal
        property QtObject someProperty: someObjectInstance ? someObjectInstance : null
    }

    property Connections c: Connections {
        target: internal
        function onSomePropertyChanged() {
              internal.someProperty = null
        }
    }
}
