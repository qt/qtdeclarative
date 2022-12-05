import QtQml

QtObject {
    component AccessibleButton : QtObject {
        required property string description
        objectName: description
    }
    property AccessibleButton a: AccessibleButton {}
    property AccessibleButton b: AccessibleButton {
        description: "b"
    }
}
