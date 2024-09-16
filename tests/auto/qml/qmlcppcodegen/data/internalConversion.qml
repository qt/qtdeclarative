pragma Strict
import QtQml

QtObject {
    property QtObject offset: QtObject {
        id: a
        property string mark
    }

    function markInputs(mark: string) {
        offset.objectName = mark;
        a.mark = mark;
    }

    Component.onCompleted: markInputs("hello")
}
