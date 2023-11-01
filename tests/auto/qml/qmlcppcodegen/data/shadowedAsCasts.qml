pragma Strict
import QtQml

QtObject {
    property ShadowedObjectName shadowed1: ShadowedObjectName {}
    property ShadowedObjectName shadowed2: ShadowedObjectName {}
    property QtObject shadowed3: ShadowedObjectNameDerived {}

    function returnShadowed2() : QtObject { return shadowed2 }

    function a(mark: int) {
        // as-cast can be optimized out if we're clever.
        (shadowed1 as QtObject).objectName = mark;
    }

    function b(mark: int) {
        // method return values can contain shadowed properties!
        returnShadowed2().objectName = mark;
    }

    function c(mark: int) {
        // Has to do an actual as-cast, but results in ShadowedObjectNameDerived!
        (shadowed3 as ShadowedObjectName).objectName = mark;
    }

    Component.onCompleted: {
        a(43);
        b(42);
        c(41);
    }
}
