import QtQml
import QtQml.Models

QtObject {
    id: root

    property ListModel listModel: ListModel {
        id: lm
        dynamicRoles: true
    }

    Component.onCompleted: {
        var depth = 200000
        var obj = { a: [] }
        var cursor = obj
        for (var i = 0; i < depth; ++i) {
            var next = { a: [] }
            cursor.a.push(next)
            cursor = next
        }
        lm.append(obj)
    }
}