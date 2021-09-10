import QtQuick
Item {
    component RequiredPropertyType : QtObject {
        required property int i
        i: 42
    }

    RequiredPropertyType {} // should succeed
}
