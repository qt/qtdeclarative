import QtQml 2.0

QtObject {
    property int signal1P: 0
    signal signal1
    onSignal1: function() {
        signal1P++;
    }

    property string signal2P1: ""
    property int signal2P2: 0
    property string signal2P3: ""
    signal signal2(string x, int y)
    onSignal2: function (x, y) {
        signal2P1 = x;
        signal2P2 = y;
        signal2P3 = x + y;
    }

    function qmlEmitSignal1() { signal1(); }
    function qmlEmitSignal2() { signal2("xyz", 123); }
    function qmlEmitSignal2WithArgs(x, y) { signal2(x, y); }
}
