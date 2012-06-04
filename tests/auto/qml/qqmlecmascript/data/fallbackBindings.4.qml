import QtQuick 2.0
import Qt.test.fallbackBindingsDerived 1.0 as ModuleAPI

Item {
    property bool success: false
    property string foo: ModuleAPI.test

    Component.onCompleted: success = (foo == 'hello')
}
