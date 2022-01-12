
import QtQuick 2.0
import QtTest 1.1

Canvas {
    id: canvas
    width: 1
    height: 1
    contextType: "2d"

    property var contextInPaint

    onPaint: {
        contextInPaint = context;
    }

    TestCase {
        name: "ContextTypeStored"
        when: windowShown

        function test_contextType() {
            compare(canvas.contextType, "2d");
        }
    }
}
