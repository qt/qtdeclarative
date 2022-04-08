import QtQml 2.15
QtObject {
    function getConstantValue() { return 42; }
    function squareValue(x) { return x * x; }
    function concatenate(str1, str2) { return str1 + str2; }

    function touchThisAndReturnSomething(x: int) {
        this.bar++;
        return x + 1;
    }

    property int foo: 42
    property int bar: 0
    property int baz: -100
    onFooChanged: () => {
        var value = touchThisAndReturnSomething(foo);
        bar += value;
    }
    onBazChanged: (y) => {
        var value = touchThisAndReturnSomething(this.baz + y);
        this.bar = value;
    }
}
