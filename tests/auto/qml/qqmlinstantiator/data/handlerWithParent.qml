import QtQuick

Item {
    id: root
    Instantiator {
        model: 2
        delegate: PointHandler {
            objectName: "pointHandler"
            parent: root
        }
    }
}
