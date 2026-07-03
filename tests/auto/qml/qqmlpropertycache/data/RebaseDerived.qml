import QtQml

// A type deriving from RebaseBase that adds its own property (with a change signal),
// its own signal and its own method — one own member in each index space.
RebaseBase {
    property int own: 7
    signal ownSignal()
    function ownMethod() {}
}
