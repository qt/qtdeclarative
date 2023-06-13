import QtQuick

Rectangle {
    width: 300
    height: 300

    // test SequentialAnimation
    Rectangle {
        id: line0
        y: 100
        width: parent.width
        height: 2
        color: "blue"
    }
    Rectangle {
        id: target0
        objectName: "target0"
        y: 100
        anchors.verticalCenter: line0.verticalCenter
        height: line0.height * 5
        width: height
        color: "red"
        radius: height/2

        property bool onFinishedCalled : false;

        SequentialAnimation {
            id: seqAnim0
            objectName: "seqAnim0"
            loops: 2
            running: true
            NumberAnimation {
                id: anim0
                target: target0
                property: "x"
                from: 0
                to: 50
                duration: 500
            }
            Component.onCompleted: anim0.to = 290
            onFinished: target0.onFinishedCalled = true
        }
    }

    // test ParallelAnimation
    Rectangle {
        id: line1
        y: 200
        width: parent.width
        height: 2
        color: "blue"
    }
    Rectangle {
        id: target1
        objectName: "target1"
        anchors.verticalCenter: line1.verticalCenter
        height: line1.height * 5
        width: height
        color: "yellow"
        radius: height/2

        property bool onFinishedCalled : false;

        ParallelAnimation {
            id: parAnim0
            objectName: "parAnim0"
            loops: 2
            running: true
            NumberAnimation {
                id: anim1
                target: target1
                property: "x"
                from: 0
                to: 50
                duration: 500
            }
            Component.onCompleted: anim1.to = 290
            onFinished: target1.onFinishedCalled = true
        }
    }

    Timer {
        interval: 400
        running: true
        onTriggered: {
            seqAnim0.pause()
            parAnim0.pause()
            anim0.to = 140
            anim1.to = 140
        }
    }
}
