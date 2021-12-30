import QtQuick
import QmltcTests 1.0
TypeWithDeferredComplexProperties {
    group.str: "still immediate"
    group.deferred: -1

    DeferredAttached.attachedFormula: Math.abs(10 * 2)
    DeferredAttached.deferred: 100
}
