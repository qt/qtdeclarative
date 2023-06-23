pragma Strict
import QtQml

QtObject {
    id: win
    property real width: 640
    property real height: 480
    property string title: "none"

    function t1(): void {
        const w = win.width
        const h = win.height

        if (w > 0 && h > 0) {
            const wByH = h / 3.0 * 4.0
        }
    }

    function t2(): void {
        let i = 42
        win.title = "Foo " + i

        for (let j = 0; j < 10; j++) {
            win.title = "Bar " + j
        }

        for (let k = 0; k < i; k++) {
            win.title = "Baz " + k
        }
    }

    function t3(): void {
        let v1 = 1
        let v2 = 2

        if (true) {
            v1 = v2
        }
    }
}
