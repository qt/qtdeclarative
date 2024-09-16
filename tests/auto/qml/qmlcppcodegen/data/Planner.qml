pragma Strict
import QtQml

QtObject {
    id: planner
    property Variable last: Variable { value: 10 }

    function newMark() : int {
        return 5;
    }

    function addPropagate(i: int) : bool {
        return false;
    }

    function typeErasedRemoveOne(v: QtObject) { removeOne(v as Variable) }

    // Work with various shadowable members and return values.
    function removeOne(v: Variable) {
        let vDeterminedBy = v.determinedBy;
        for (let i = 0, length = v.length(); i < length; ++i) {
            let next = v.constraint(i) as BaseConstraint;
            if (next.satisfaction === Satisfaction.NONE)
                objectName += "n"
            else if (next !== vDeterminedBy)
                objectName += "d"
            else
                objectName += "x"
        }
    }

    function typeErasedRun(c: QtObject) { run(c as BaseConstraint) }

    function run(initial: BaseConstraint) {
        let mark = planner.newMark();
        let c = initial;

        let output = c.output as Variable;
        if (output.mark !== mark && c.inputsKnown(mark)) {
            output.mark = mark;
        }
    }

    function verify(i: int) {
        if (last.value !== i)
            console.error("failed", last.value, i);
        else
            console.log("success")
    }
}
