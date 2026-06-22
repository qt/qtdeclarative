import QtQuick
import QmlBench

// Tests the creation of items with a non-recursive function binding
CreationBenchmark {
    id: root;
    count: 50;
    staticCount: 1000;
    delegate: Item {
        x: sum(10000);
    }

    function sum(n) {
        var x = 0;
        for (var i=0; i<n; ++i)
            x = x + 1;
        return x;
    }
}

