import QtQuick
import qt.test 1.0
Item {
    component RequiredPropertyType : TwoRequiredProperties {
        index: 11
        name: "foobar"
    }
    RequiredPropertyType {} // should succeed
}
