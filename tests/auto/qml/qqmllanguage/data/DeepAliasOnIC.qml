import QtQml

QtObject {
    id: root

    component ObjectWithColor: QtObject {
        property string color
        property var varvar
    }

    property ObjectWithColor border: ObjectWithColor {
        color: root.trueBorderColor
        varvar: root.trueBorderVarvar
    }

    readonly property rect readonlyRect: ({x: 12, y: 13, width: 14, height: 15})

    property alias borderColor: root.border.color
    property alias borderVarvar: root.border.varvar
    property alias readonlyRectX: root.readonlyRect.x

    property string trueBorderColor: "green"
    property var trueBorderVarvar: 1234
}
