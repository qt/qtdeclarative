import QtQml
import QtQml.Models

// Mechanism: QQmlListModelPrivate::canMove(from, to, n) (qqmllistmodel_p.h:105)
// checks !(from+n > count() || to+n > count() || from < 0 || to < 0 || n < 0).
// For from = INT_MAX (2147483647) and n = 1, "from+n" is a signed integer
// overflow. In practice this wraps to INT_MIN, and INT_MIN > count() is
// false for any realistic count() -- so the check passes despite "from"
// being nowhere near a valid index.
QtObject {
    id: root

    property ListModel listModel: ListModel {
        id: lm
        dynamicRoles: true   // must be set before any data is added
    }

    Component.onCompleted: {
        lm.append({ value: 1 })
        console.log("[reproducer] count before move:", lm.count)
        console.log("[reproducer] calling lm.move(2147483647, 0, 1) -- from+n overflows INT_MAX in canMove()'s bounds check")
        lm.move(2147483647, 0, 1)
        console.log("[reproducer] survived without a crash -- count after move:", lm.count)
    }
}