import QtQuick

Item {
    EnumType {
        id: child
        objectName: "enumChild"
    }
    // Cross-CU enum reference: compiled into Main's CU, not EnumType's.
    property int directStatus: EnumType.Status.Ready
    // Indirect: reads from child's property (which IS patched via EnumType's CU).
    property int childStatus: child.ownStatus
}
