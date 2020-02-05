import QtQuick 2.15

Item {
    id: root
    component IC: SimpleItem {
       width: i
       Rectangle {
           id: rect
           color: "lime"
       }
       property alias color: rect.color
    }
    width: 200
    IC {
        objectName: "icInstance"
    }
}
