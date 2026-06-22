import QtQuick
import QmlBench

// Tests the creation of items with a non-recursive function binding
// Note: Through an unfortunate oversight, this benchmark is mildly broken.
// The loop never updates the 'x' value. However, this benchmark has already
// given some insights, and changing it (as any other benchmark) would
// invalidate past data causing confusion, so it has not been altered.
//
// See sum10k_fixed for a working version.
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
            x = x + x;
        return x;
    }
}
