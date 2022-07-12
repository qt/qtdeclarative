pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: timeMarks

    property QtObject model
    property int rowCount: model ? model.rowCount : 0

    Connections {
        target: timeMarks.model
        function onExpandedRowHeightChanged() {
            console.log("necessary")
        }
    }

    Repeater {
        id: rowRepeater
        model: timeMarks.rowCount
        Rectangle {
            objectName: "zap"
            // required property int index <-- would prevent the crash
        }
    }
}
