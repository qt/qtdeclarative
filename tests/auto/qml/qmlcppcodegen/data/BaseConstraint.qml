pragma Strict
import QtQml

QtObject {
    property int satisfaction: Satisfaction.NONE
    property QtObject output

    function inputsKnown(mark: int) : bool { return true }
}
