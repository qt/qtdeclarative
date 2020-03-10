import QtQuick 2.15

Item {
    id: root
    component IC: SimpleItem {
       id: root
       width: root.i
       property color color: "red"
    }
    width: 200
    IC {
        objectName: "icInstance"
    }
}
