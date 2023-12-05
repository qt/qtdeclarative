pragma Strict
import QtQml

QtObject {
    id: variable
    property int value: 0
    property int mark: 0
    property BaseConstraint determinedBy: null
    property list<BaseConstraint> constraints: [
        BaseConstraint {
            satisfaction: variable.value == 0 ? Satisfaction.NONE : Satisfaction.FORWARD
        }
    ]

    function length(): int {
        return constraints.length
    }

    function constraint(i: int) : BaseConstraint {
        return constraints[i];
    }
}
