import test
import QtQuick

Item {
    id: root
    property bool toggle: true
    property int counter: 0

    x: toggle ? 100 : tester.objectProperty.x

    DeferredPropertyTester {
        id: tester
	objectProperty: Item {  x: { console.log("hi");  return 300; } }
    }
}
