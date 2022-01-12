
import QtQuick 2.0
import QtTest 1.1

Canvas {
    id: canvas
    width: 1
    height: 1
    contextType: "2d"

    property var contextInPaint

    SignalSpy {
        id: contextSpy
        target: canvas
        signalName: "contextChanged"
    }

    onPaint: {
        contextInPaint = context;
    }

    TestCase {
        name: "Colors"
        when: canvas.available

        function test_colors() {
            wait(100);
            compare(contextSpy.count, 1);

            var ctx = canvas.getContext("2d");
            // QTBUG-47894
            ctx.strokeStyle = 'hsl(255, 100%, 50%)';
            var c1 = ctx.strokeStyle.toString();
            ctx.strokeStyle = 'hsl(320, 100%, 50%)';
            var c2 = ctx.strokeStyle.toString();
            verify(c1 !== c2);
        }
    }
}
