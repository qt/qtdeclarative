import QtQml

QtObject {
    id: self
    property var selfAsBroken: self as Broken
}
