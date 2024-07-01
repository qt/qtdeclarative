import QtQml

QtObject {

    property Component object: QtObject {
        property QtObject obj
    }

    property QtObject outer
    property QtObject inner

    Component.onCompleted: {
        inner = object.createObject(this, { obj: null })
        outer = object.createObject(this, { obj: Qt.binding(() => inner) })
    }
}
