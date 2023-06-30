pragma Strict
import QtQml

QtObject {
    property rect r: ({x: 1, y: 2, width: 3, height: 4})
    property rect r2: { var x = 42; return {x}; }
    property weatherModelUrl w: ({ strings: ["one", "two", "three"] })
}
