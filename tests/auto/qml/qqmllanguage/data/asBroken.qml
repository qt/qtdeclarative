import QtQml 2.15

QtObject {
    id: self
    property var selfAsBroken: self as Broken
}
