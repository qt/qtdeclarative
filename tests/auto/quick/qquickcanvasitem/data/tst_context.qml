
import QtQuick 2.0
import QtTest 1.0

Canvas {
    id: canvas
    width: 1
    height: 1
    contextType: "2d"

    property var contextInPaint

    SignalSpy {
        id: paintedSpy
        target: canvas
        signalName: "paint"
    }

    SignalSpy {
        id: contextSpy
        target: canvas
        signalName: "contextChanged"
    }

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

    TestCase {
        name: "ContextValidWhenTypePredefined"
        when: canvas.available

        function test_context() {
            // Wait for the context to become active
            wait(100);
            compare(contextSpy.count, 1);

            // Context is available
            verify(canvas.context)
        }

        function test_contextIsConsistent() {
            // Wait for the context to become active
            wait(100);
            compare(contextSpy.count, 1);

            // getContext("2d") is the same as the context property
            compare(canvas.getContext("2d"), canvas.context);
        }

        function test_paintHadContext() {
            // Make there was a paint signal
            wait(100);
            verify(paintedSpy.count, 1)

            // Paint was called with a valid context when contextType is
            // specified
            verify(canvas.contextInPaint)

            // paints context was the correct one
            compare(canvas.contextInPaint, canvas.getContext("2d"));
        }
   }
}
