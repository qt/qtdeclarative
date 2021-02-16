import QmltcTests 1.0
import QtQml 2.0

QmlGroupPropertyTestType {
    group.count: 42
    group.formula: 41 + 1
    group.object: QtObject {
        property string name: "root.group.object"
    }

    property int myCount: group.count
    property bool myTriggerFired: false

    signal myTriggered()
    onMyTriggered: function() { group.triggered(); }

    group.onTriggered: function() {
        myTriggerFired = true;
        updateCount();
    }

    group.onCountChanged: {
        group.formula = group.formula * 2;
    }

    signal updateCount()
    onUpdateCount: function() { group.count++; }
}
