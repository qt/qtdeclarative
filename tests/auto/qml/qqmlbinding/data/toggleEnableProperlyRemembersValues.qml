import QtQml

QtObject {
    id: root
    property bool enabled: false
    property var func: function() { return 1 }
    property var arr: [1, 2]
    property Binding b: Binding {
        root.func: function() { return 2 };
        root.arr: [1, 2, 3]
        when: root.enabled
    }
}
