import QmltcTests 1.0
import QtQml 2.0

QtObject {
    id: root
    TestType.attachedCount: 42
    TestType.attachedFormula: 41 + 1
    TestType.attachedObject: QtObject {
        property string name: "root.TestType.attachedObject"
    }

    property int myCount: root.TestType.attachedCount
    property bool myTriggerFired: false

    signal myTriggered()
    onMyTriggered: function() { root.TestType.triggered(); }

    TestType.onTriggered: function() {
        root.myTriggerFired = true;
        updateAttachedCount();
    }

    TestType.onAttachedCountChanged: {
        root.TestType.attachedFormula = root.TestType.attachedFormula * 2;
    }

    signal updateAttachedCount()
    onUpdateAttachedCount: function() { root.TestType.attachedCount++; }
}
