import QtQuick
TreeView {
    Component.onCompleted: {
        expandRecursively()
        collapseRecursively()
    }
}
