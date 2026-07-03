import QtQml

// Same members as OverrideBase.qml, but declared in the opposite order, mimicking a
// base type whose *existing* members changed index across a hot reload (not a pure
// append at the end). "a" moves from property index 1 to 2; "b" takes index 1.
QtObject {
    property int b: 2
    property int a: 1
}
