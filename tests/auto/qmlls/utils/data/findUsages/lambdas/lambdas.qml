import QtQuick

Item {
    signal helloSignal()

    onHelloSignal: function(a, b, c) {
        let x = a + b, y = b + c;
        let moreLambdas = (x, y)  => x + y + a;
        moreLambdas(a + b, b - c);
        let r = a + b / c;
    }
}
