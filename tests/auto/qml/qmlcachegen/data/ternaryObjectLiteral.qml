import QtQml

QtObject {
    property bool cond: true
    readonly property var objects: cond ? {a: 1} : {a: 2}
}
