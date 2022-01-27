import QtQml
QtObject {
    property string myProperty: "foobar"

    signal mySignal()

    property int conflictingProperty: 42
    signal conflictingPropertyChanged(a: real, c: string)
}
