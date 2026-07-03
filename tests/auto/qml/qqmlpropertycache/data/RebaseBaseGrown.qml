import QtQml

// Same as RebaseBase.qml with one extra property (and thus one extra change signal)
// appended, mimicking a base type that grew across a hot reload.
QtObject {
    property int b0
    property int b1
    property int b2
}
