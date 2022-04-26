import QtQml

QtObject {
    component View: QtObject {
        default property Component delegate
    }

    component Things : QtObject {
        property QtObject view: View { delegate: QtObject {} }
    }

    component Delegated : View {
        delegate: QtObject {}
    }

    property Things things: Things {}
    property Delegated delegated: Delegated {}
}
