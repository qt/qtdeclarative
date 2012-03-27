import QtQuick 2.0

import Qt.test.legacyModuleApi 1.0 as ModApi // was registered with non-templated function

QtObject {
    property int legacyModulePropertyTest: ModApi.qobjectTestProperty
    property int legacyModuleMethodTest

    Component.onCompleted: {
        legacyModuleMethodTest = ModApi.qobjectTestMethod();
    }
}

