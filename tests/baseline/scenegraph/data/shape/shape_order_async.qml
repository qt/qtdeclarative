import QtQuick

Item {
    width: 320
    height: 480
    property bool suspendGrabbing: paths.isAnimating

    OrderedPaths {
        id: paths
        anchors.fill: parent
        async: true
    }
}

