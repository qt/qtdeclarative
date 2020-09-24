import QtQml

QtObject {
    // Remarkably, Easing.Type.Linear is not allowed in QML
    // This is in contrast to QML-declared enums. See EnumAccess2.qml
    property int foo: Easing.Linear
}

