import Qt.test 1.0 as ModApi
import QtQuick 2.0

Item {
    property int testProp: ModApi.ip
    property int testProp2: 2

    function getRandom() {
        testProp2 = ModApi.random();
        // testProp should also have changed.
    }
}
