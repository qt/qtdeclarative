pragma Strict
import QtQml
import TestTypes

Person {
    property list<var> varlist: [1, "foo", this, undefined, true]
    property list<QtObject> objlist: [this, null, this]
}
