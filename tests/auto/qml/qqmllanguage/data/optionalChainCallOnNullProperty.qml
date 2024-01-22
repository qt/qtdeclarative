import QtQml

QtObject {
    id: root
    property QtObject target: null

    Component.onCompleted: {
        target?.destroy( )
    }
}
