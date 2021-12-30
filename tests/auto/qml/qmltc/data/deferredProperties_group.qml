import QtQuick
import QmltcTests 1.0
TypeWithDeferredGroup {
    group.str: "foo" + "bar"
    group.deferred: 42
}
