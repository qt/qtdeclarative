pragma Strict
import QtQml

QtObject {
    property int v: 1
    property int w: 1
    property int x: 1
    property int y: 1
    property int z: 1
    Component.onCompleted: {
        var g = null;
        if (g !== null) {
            v = 2;
        }
        if (g === null) {
            w = 3;
        }

        var h = undefined;
        if (h !== undefined) {
            x = 4;
        }
        if (h === undefined) {
            y = 5;
        }

        var o = this;
        if (o != null)
            z += 7;
        if (o == null)
            z += 6;
        if (g == null)
            z += 10;
        if (g != null)
            z += 20;
    }
}
