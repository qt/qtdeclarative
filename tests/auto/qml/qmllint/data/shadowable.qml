import QtQml

QtObject {
    property NotSoSimple a: NotSoSimple {}
    property int b: a.complicated
}
