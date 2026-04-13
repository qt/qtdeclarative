import QtQml

QtObject {
    id: root

    component Inner : QtObject {
        property int value: 42
    }

    property alias deepValue: root.innerObj.value
    property Inner innerObj: Inner {}
    property QtObject other: QtObject {
        property alias proxy: root.deepValue
    }
}
