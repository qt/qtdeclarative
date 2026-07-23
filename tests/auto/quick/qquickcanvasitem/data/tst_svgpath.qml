import QtQuick 2.0

CanvasTestCase {
    id:testCase
    name: "svgpath"
    function init_data() { return testData("2d"); }
    function test_svgpath(row) {
        var canvas = createCanvasObject(row);
        tryVerify(function() { return canvas.available; });
        var ctx = canvas.getContext('2d');
        var svgs = [
                    // Absolute coordinates, explicit commands.
                    "M50 0 V50 H0 Q0 25 25 25 T50 0 C25 0 50 50 25 50 S25 0 0 0 Z",
                    // Absolute coordinates, implicit commands.
                    "M50 0 50 50 0 50 Q0 25 25 25 Q50 25 50 0 C25 0 50 50 25 50 C0 50 25 0 0 0 Z",
                    // Relative coordinates, explicit commands.
                    "m50 0 v50 h-50 q0 -25 25 -25 t25 -25 c-25 0 0 50 -25 50 s0 -50 -25 -50 z",
                    // Relative coordinates, implicit commands.
                    "m50 0 0 50 -50 0 q0 -25 25 -25 25 0 25 -25 c-25 0 0 50 -25 50 -25 0 0 -50 -25 -50 z",
                    // Absolute coordinates, explicit commands, minimal whitespace.
                    "m50 0v50h-50q0-25 25-25t25-25c-25 0 0 50-25 50s0-50-25-50z",
                    // Absolute coordinates, explicit commands, extra whitespace.
                    " M  50  0  V  50  H  0  Q 0  25   25 25 T  50 0 C 25   0 50  50 25 50 S  25 0 0  0 Z"
                   ];

        var blues = [
                   -1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    1, 1, 0, 0, 0, 0, 0, 0,-1, 1,
                    1, 1, 1, 0, 0, 0, 0, 0, 1, 1,
                    1, 1, 1, 0, 0, 0, 0, 0, 1, 1,
                    1, 1, 1, 0, 0, 0, 0, 0, 1, 0,
                    1, 1, 1, 0,-1, 1, 1, 1, 0, 0,
                    1, 1, 0, 1, 1, 1, 1, 1, 0, 0,
                    1, 0, 0, 1, 1, 1, 1, 1, 0, 0,
                    1, 0, 0, 1, 1, 1, 1, 1, 0, 0,
                   -1, 0, 0, 0, 1, 1, 1, 0, 0, 0
                   ];

        ctx.fillRule = Qt.OddEvenFill;
        for (var i = 0; i < svgs.length; i++) {
            ctx.fillStyle = "blue";
            ctx.fillRect(0, 0, 50, 50);
            ctx.fillStyle = "red";
            ctx.path = svgs[i];
            ctx.fill();
            var x, y;
            for (x=0; x < 10; x++) {
                for (y=0; y < 10; y++) {
                    if (blues[y*10 +x] == -1) continue; //edge point, different render target may have different value
                    if (blues[y * 10 + x]) {
                        comparePixel(ctx, x * 5, y * 5, 0, 0, 255, 255);
                    } else {
                        comparePixel(ctx, x * 5, y * 5, 255, 0, 0, 255);
                    }
                }
            }
        }
    }

    // The test only verifies that assigning malformed strings (esp those ending
    // with whitespace) to ctx.path does not crash the parser.
    function test_svgpath_malformed_data() {
        return [
            { tag: "empty", path: "" },
            { tag: "single space", path: " " },
            { tag: "multiple spaces", path: "   " },
            { tag: "abs and newlines", path: "\t\n "},
            { tag: "command then trailing whitespace", path: "M0 0   "},
            { tag: "trailing whitespace after args", path: "M0 0 L10 10 \n "},
            { tag: "whitespace after command to end", path: "L \t"},
            { tag: "whitespace between command and numbers", path: "M  10  20  "},
            { tag: "trailing comma to end", path: "M0 0,"},
            { tag: "comma then whitespace to end", path: "M0 0, \n"},
            { tag: "comma between numbers then end", path: "L10,10, "},
            { tag: "integer to end", path: "M0 0 L10 20"},
            { tag: "decimal to end", path: "M0 0 L1.5 2.5"},
            { tag: "trailing dot to end", path: "M0 0 L1. 2."},
            { tag: "exponent to end", path: "M0 0 L1e2 3e2"},
            { tag: "signed exponent to end", path: "M0 0 L1e-2 3e-2"},
            { tag: "bare exponent letter to end", path: "M0 0 L1e"},
            { tag: "sign only token to end", path: "M0 0 L-"},
            { tag: "dot only token to end", path: "M0 0 L."}
        ]
    }

    function test_svgpath_malformed(data) {
        var canvas = Qt.createQmlObject(`
            import QtQuick
            Canvas {
                height: 100
                width:100
                renderTarget:Canvas.Image
            }
        `, testCase, "testCanvas");
        tryVerify(function() { return canvas.available; });
        var ctx = canvas.getContext('2d');
        ctx.beginPath();
        ctx.path = data.path;
        ctx.fill();
        verify(true); // reached here without crashing
    }
}
