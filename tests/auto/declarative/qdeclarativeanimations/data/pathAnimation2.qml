import QtQuick 2.0

Rectangle {
    width: 400
    height: 400

    Rectangle {
        id: redRect
        color: "red"
        width: 100; height: 100
        x: 50; y: 50
    }

    PathAnimation {
        target: redRect
        duration: 100;
        endRotation: 0
        orientationEntryInterval: .1
        orientationExitInterval: .1
        orientation: PathAnimation.RightFirst
        path: Path {
            startX: 50; startY: 50
            PathLine { x: 300; y: 300 }
        }
    }
}
