import QtQuick
import qt.test 1.0
Item {
    component RequiredPropertyType : RequiredTwoPropertiesSet { }
    RequiredPropertyType {} // should succeed
}
