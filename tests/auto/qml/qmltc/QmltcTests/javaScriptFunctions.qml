import QtQml 2.0

QtObject {
    property int func1P: 0
    function func1() {
        func1P++;
    }

    property string func2P: ""
    function func2(x) {
        func2P = x;
    }

    property bool func3P: false
    function func3() {
        return func3P;
    }
}
