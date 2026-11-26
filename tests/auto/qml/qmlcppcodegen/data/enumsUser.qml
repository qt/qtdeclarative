pragma Strict
import QtQml
import TestTypes

QtObject {
    function getEnum(): int {
        return Test.AA + Test.BB + Test.CC + Test.DD
    }
}
