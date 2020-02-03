import QtQuick 2.15

Item {
    readonly property QtObject xBehaviorObject: xBehavior.targetProperty.object
    readonly property string xBehaviorName: xBehavior.targetProperty.name
    readonly property QtObject emptyBehaviorObject: emptyBehavior.targetProperty.object
    readonly property string emptyBehaviorName: emptyBehavior.targetProperty.name
    Behavior on x {
        id: xBehavior
        objectName: "xBehavior"
    }
    Behavior {
        id: emptyBehavior
        objectName: "emptyBehavior"
    }
}
