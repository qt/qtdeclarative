import QtQml
QtObject {
    signal justSignal()
    signal typedSignal(string a, QtObject b, real c)

    function justMethod() {
        console.log("justMethod()");
    }

    function untypedMethod(d, c) {
        console.log("methodWithParams, d = " + d + ", c = " + c);
    }

    function typedMethod(a: real, b: int): string {
        console.log("typedMethod, a  = " + a + ", b = " + b);
        return a + b;
    }
}
