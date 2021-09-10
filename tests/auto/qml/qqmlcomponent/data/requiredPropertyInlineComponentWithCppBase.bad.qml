import QtQuick
import qt.test 1.0
Item {
    component RequiredPropertyType : TwoRequiredProperties {
        index: 0
        // but name is unset
    }
    RequiredPropertyType {} // should fail (name is not initialized)
}
