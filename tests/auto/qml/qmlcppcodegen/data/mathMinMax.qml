pragma Strict
import QtQml
import QtQuick

Rectangle {
    Component.onCompleted: {
        // Math.max()
        console.log(Math.max(1, 1));
        console.log(Math.max(1, 2));
        console.log(Math.max(2, 1));
        console.log(Math.max(0, 0));
        console.log(Math.max(-1, 0));
        console.log(Math.max(0, -1));
        console.log(Math.max(-1, -1));

        console.log(Math.max(0, 0, 0));
        console.log(Math.max(0, 0, 1, 0, 0, 0));
        console.log(Math.max(-2, -1, 0, 1, 2));
        console.log(Math.max(2, 1, 0, -1, -2));
        console.log(Math.max(9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0));

        console.log(Math.max(0.0, 0.0, 0.0))
        console.log(Math.max(-0.001, 0.001, 0.002))
        console.log(Math.max(5.4, 1, 0.002))
        console.log(Math.max(null, 0, -1, 8E-2, NaN, undefined, true, false, Infinity))
        console.log(Math.max(0, -1, 8E-2, true, false, Infinity))
        console.log(Math.max(0, -1, 8E-2, true, false))
        console.log(Math.max(0, -1, 8E-2, false))
        console.log(Math.max(0, -1, 8E-2, true, false, Infinity))
        console.log(Math.max(-1, -8, null))
        console.log(Math.max(undefined, 20, 70))

        // Math.min()
        console.log(Math.min(1, 1));
        console.log(Math.min(1, +2));
        console.log(Math.min(2, 1));
        console.log(Math.min(0, 0));
        console.log(Math.min(-1, 0));
        console.log(Math.min(0, -1));
        console.log(Math.min(-1, -1));

        console.log(Math.min(0, 0, 0));
        console.log(Math.min(0, 0, 1, 0, 0, 0));
        console.log(Math.min(-2, -1, 0, 1, 2));
        console.log(Math.min(2, 1, 0, -1, -2));
        console.log(Math.min(9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0));

        console.log(Math.min(0.0, 0.0, 0.0))
        console.log(Math.min(-0.001, 0.001, 0.002))
        console.log(Math.min(5.4, 1, 0.002))
        console.log(Math.min(null, 0, -1, 8E-2, NaN, undefined, true, false, Infinity))
        console.log(Math.min(0, -1, 8E-2, true, false, Infinity))
        console.log(Math.min(0, -1, 8E-2, true, false))
        console.log(Math.min(0, -1, 8E-2, false))
        console.log(Math.min(0, -1, 8E-2, true, false, Infinity))
        console.log(Math.min(-1, -8, null))
        console.log(Math.min(undefined, 20, 70))
    }
}
