import QtQuick 2.0
Rectangle {
    width: 400
    height: 400
    Rectangle {
        id: rect
        objectName: "MyRectOneWay"
        width: 100; height: 100; color: "green"
        Behavior on x {
            id: behavior
            objectName: "MyBehaviorOneWay";
            enabled: (behavior.targetValue === 0)
            NumberAnimation {
                id: ani
                objectName: "MyAnimationOneWay";
                duration: 200;
            }
        }
    }
}
