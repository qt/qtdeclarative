import QtQuick
import QmlBench

// Tests the creation of Items with a recursive function binding
CreationBenchmark {
    id: root;
    count: 50;
    staticCount: 1000;
    delegate: Item {
        x: fib(10);
    }

    function fib(n) {
        if (n < 2)
            return Math.max(0, n);
        else
            return fib(n-1) + fib(n-2);
    }
}
