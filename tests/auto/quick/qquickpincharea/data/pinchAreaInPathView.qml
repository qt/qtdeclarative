import QtQuick

PathView {
    width: 600
    height: 200

    model: 3
    delegate: Rectangle {
        width: 200
        height: 200
        color: "salmon"
        opacity: PathView.isCurrentItem ? 1 : 0.5

        property alias pinchArea: pinchArea

        Text {
            text: "Test"
            font.pixelSize: 100
            anchors.fill: parent
            fontSizeMode: Text.Fit
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        PinchArea {
            id: pinchArea
            anchors.fill: parent
            pinch.target: parent
            pinch.dragAxis: Pinch.XAndYAxis
            pinch.minimumScale: 1.0
            pinch.maximumScale: 5.0

            onPinchFinished: (pinch) => {
                parent.scale = 1
                parent.x = 0
                parent.y = 0
            }
        }
    }
    path: Path {
        startX: 100
        startY: 100
        PathLine {
            x: 700
            y: 100
        }
    }
}
