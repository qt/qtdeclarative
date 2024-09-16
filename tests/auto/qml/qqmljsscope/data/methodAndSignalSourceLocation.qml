import QtQml

QtObject {
    function f1() { }
    function f2(a) { }
    function f3(a: int) { }
    function f4(a, b) { }
    function f5(a, b): void { }
    function f6(a, b, c): void {
        // Nothing
    }

    signal s1()
    signal s2(a: int)
    signal s3(a: int, b: string)
}
