import "QTBUG-45916.js" as JSTest
import QtQuick

Item {
    id: root

    property var foo: JSTest.foo("Test")
}
