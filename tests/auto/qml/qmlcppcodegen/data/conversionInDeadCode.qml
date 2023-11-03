pragma Strict
import QtQml

QtObject {
    // This does not look like dead code, but each access to 'result' generates a
    // DeadTemoralZoneCheck instruction that we ignore when compiling to C++
    // after checking statically that 'result' is alive throughout the function.
    // Therefore, this function is a torture test for the dead code elimination.
    function calc(a: int, b: int) : int {
        let result = a;
        if (b < 0) {
            if (b < -1)
                result -= b;
            if (b < -2)
                result /= b;
        } else {
            if (b > 1)
                result *= b;
            if (b > 2)
                result += b;
        }
        return result;
    }

    property int a: calc(10, -3);
    property int b: calc(10, -2);
    property int c: calc(10, -1);
    property int d: calc(10, 0);
    property int e: calc(10, 1);
    property int f: calc(10, 2);
    property int g: calc(10, 3);
}
