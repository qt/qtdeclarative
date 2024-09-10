import QtQuick

Item {
    function f(a: int, b: string): bool {}
    signal f2(int a, b: string)
    signal noArgs()
    function returnVoid(): void {}
    function defaultArgs(x = 12345, y = { x: 44, y: "hello", z: x => x }): void {}
    function deconstruction({ x = 4 }): void {}
    function /*f*/comments(/*a*/a/*a*/: /*int*/int/*int*/, /*b*/b/*b*/): /*int*/int/*int*/ {}
}
