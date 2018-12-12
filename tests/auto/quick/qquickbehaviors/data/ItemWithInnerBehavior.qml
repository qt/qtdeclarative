import QtQuick 2.4

Item {
    id: root

    property bool someValue
    Behavior on someValue {
        objectName: "behavior"
        ScriptAction { script: { parent.behaviorTriggered = true }}
    }
}
