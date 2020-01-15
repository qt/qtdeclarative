import QtQml 2.15

QtObject {
    property alias i: icInstance.i
    component IC : QtObject {
        property int i: 42
    }
    property QtObject ic: IC {id: icInstance}
}
