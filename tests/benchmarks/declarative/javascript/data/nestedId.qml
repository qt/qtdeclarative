// Benchmarks the cost of accessing an id in a parent context of the script.

import QtQuick 1.0

QtObject {
    id: root

    property variant object: NestedIdObject {}
    function runtest() {
        object.runtest();
    }
}

