pragma Strict
import QtQml

QtObject {
    property Component newEditConstraint: EditConstraint {}
    property Variable variable: Variable {}
    property EditConstraint edit

    function trigger() {
        change(variable, 55);
    }

    function change(v: Variable, newValue: int) {
        edit = newEditConstraint.createObject(null, {myOutput: v}) as EditConstraint;
        v.value = newValue;
    }
}
