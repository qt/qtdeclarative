import QtQml

QtObject {
    component View: QtObject {
        default property Component delegate
    }

    component Things : QtObject {
        property QtObject view: View { delegate: QtObject {} }
    }

    property Things things: Things {}
}
