pragma Strict
import QtQml
import TestTypes

QtObject {
    id: self

    property QtObject nullObject
    property QtObject nonNullObject: QtObject {}
    property QtObject sameNonNullObject: nonNullObject
    property QtObject derivedObject: Person {name: "patron"}

    property bool derivedIsNotNull: derivedObject !== null
    property bool nullObjectIsNull: nullObject === null
    property bool nonNullObjectIsNotNull: null !== nonNullObject
    property bool compareSameObjects: sameNonNullObject === nonNullObject
    property bool compareDifferentObjects: derivedObject !== nonNullObject
    property bool compareObjectWithNullObject: nullObject !== nonNullObject
}
