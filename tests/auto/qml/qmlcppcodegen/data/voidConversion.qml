import QtQml

QtObject {
    id: item
    property point p: Qt.point(20, 10)

    Component.onCompleted: {
        item.p = undefined
    }
}
