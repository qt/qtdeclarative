import QtQuick 2.0

Component {
    Component.onCompleted: {
        // Should cause an error since having either || or && on any side of the coalescing operator is banned by the specification
        var bad_lhs_and = 3 && 4 ?? 0;
    }
}
