import QtQml
import TestTypes

QtObject {
    id: root

    property int priority: Backend.gadget.VeryHigh
    property int prop2: Backend.priority

    property bool priorityIsVeryHigh: root.priority == Backend.VeryHigh

    function cyclePriority() : int {
        root.priority = Backend.gadget.High;
        return root.priority;
    }
}
