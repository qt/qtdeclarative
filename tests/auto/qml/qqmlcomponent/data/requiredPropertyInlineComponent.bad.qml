import QtQuick
Item {
    component RequiredPropertyType : QtObject {
        required property int i
    }

    RequiredPropertyType {} // should fail
}
