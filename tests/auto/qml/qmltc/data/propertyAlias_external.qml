import QtQuick

Item {
    id: root
    property alias heightAlias: root.height
    property alias parentAlias: root.parent

    Component.onCompleted: {
        root.heightAlias = 1;
    }
}
