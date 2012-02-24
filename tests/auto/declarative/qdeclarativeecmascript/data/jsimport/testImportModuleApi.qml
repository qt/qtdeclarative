import QtQuick 2.0
import "importModuleApi.js" as Script

Item {
    property variant testValue: 5

    Component.onCompleted: {
        testValue = Script.testFunc();
    }
}
