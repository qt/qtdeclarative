import QtQuick
import QmltcTests 1.0
QtObject {
    DeferredAttached.attachedFormula: 43 + 10 - (5 * 2)
    DeferredAttached.deferred: 42
}
