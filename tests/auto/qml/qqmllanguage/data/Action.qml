import QtQuick 2.12

QtObject {
    id:root
    property Item parent
    property Item displayComponent: null

    property list<QtObject> children

    readonly property var visibleChildren: {
        var visible = [];
        var child;
        for (var i in children) {
            child = children[i];
            if (!child.hasOwnProperty("visible") || child.visible) {
                visible.push(child)
            }
        }
        return visible;
    }
}
