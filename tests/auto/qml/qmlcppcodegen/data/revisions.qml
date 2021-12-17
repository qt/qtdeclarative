import QtQml 2.2

QtObject {
    property bool delayed: false
    property int gotten: 0

    function get(a: int) { gotten = a }

    property Binding b: Binding {
        function trigger() { delayed = true }
    }

    property ObjectModel m: ObjectModel {
        function trigger() { get(5) }
    }

    Component.onCompleted: {
        b.trigger()
        m.trigger()
    }
}
