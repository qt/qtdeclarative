import QtQml 2.15
QtObject {
    function getConstantValue() { return 42; }
    function squareValue(x) { return x * x; }
    function concatenate(str1, str2) { return str1 + str2; }
}
