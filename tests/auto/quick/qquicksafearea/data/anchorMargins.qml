import QtQuick

Window {
    flags: Qt.FramelessWindowHint
    width: 500; height: 500

    SafeArea.additionalMargins {
        top: 10; left: 20
        bottom: 30; right: 40
    }

    Item {
        objectName: "content"
        anchors {
            fill: parent
            // Note: We can't use margins: parent.SafeArea.margins here
            // as the anchors.margin property is a qreal, not QMarginsF
            topMargin: parent.SafeArea.margins.top
            leftMargin: parent.SafeArea.margins.left
            rightMargin: parent.SafeArea.margins.right
            bottomMargin: parent.SafeArea.margins.bottom
        }
        property var margins: SafeArea.margins

        Item {
            objectName: "child"
            anchors.fill: parent
            property var margins: SafeArea.margins
        }
    }
}
