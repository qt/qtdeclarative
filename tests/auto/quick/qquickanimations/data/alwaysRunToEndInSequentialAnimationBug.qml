import QtQuick 2.15
import QtQuick.Window 2.15

Rectangle {
    width: 100
    height: 100
    visible: true
    color: "blue"
    property bool onFinishedCalled : false;
    property bool onStoppedCalled : false;
    property int loopsMade: 0;

    Rectangle {
        id: whiteRect
        objectName: "whiteRect"
        anchors.fill: parent
        color: "white"
        opacity: 1

    }

    property var seqAnim : SequentialAnimation {
                                loops: Animation.Infinite
                                alwaysRunToEnd: true
                                NumberAnimation {
                                    target: whiteRect
                                    properties: "opacity"
                                    from: 1;
                                    to: 0.1;
                                    duration: 500
                                }
                                NumberAnimation {
                                    target: whiteRect
                                    properties: "opacity"
                                    from: 0.1;
                                    to: 1;
                                    duration: 500
                                }
                                ScriptAction {
                                    script: loopsMade++
                                }

                                onFinished: {
                                    whiteRect.opacity = 1
                                    onFinishedCalled = true
                                }

                                onStopped : {
                                    whiteRect.opacity = 1
                                    onStoppedCalled = true
                                }

                                onStarted:{
                                    onFinishedCalled = false
                                    onStoppedCalled = false
                                    loopsMade = 0
                                }
                            }
}
