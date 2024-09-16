import QtQuick

QtObject {
    id: win

    component Foo: QtObject {
        property int progress: 0
    }

    property int progress: 0
    readonly property Foo configuring: Foo {}

    Component.onCompleted: {
        win.configuring.progress = win?.progress ?? 0
     }
}
