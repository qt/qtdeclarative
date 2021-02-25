import QtQml 2.0

QtObject {
    property int counter: 0

    signal signal1
    onSignal1: function() {
        counter++;
    }

    signal signal2(int x)
    onSignal2: function(x) {
        counter = x;
    }
}
