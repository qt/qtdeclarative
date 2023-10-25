pragma Strict
import QtQml

QtObject {
    id: self
    property int i: 0
    property Planner planner: null

    function satisfy(mark: int) {
        planner.addPropagate(mark);
        i = +mark;
    }
}
