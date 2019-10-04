import QtQuick 2.0
import Test 1.0

QtObject {
    required property TimeProvider tp
    Component.onCompleted: {
        var t = tp.time;
        tp.time = t;
    }
}
