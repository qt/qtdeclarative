import QtQml

QtObject {
    id: self
    objectName: "self"

    component Inner : QtObject {
        property QtObject shadowable: QtObject {
            objectName: "shadowable"
        }
    }

    component Outer : QtObject {
        property Inner inner: Inner {}
    }

    component Evil : Outer {
        property string inner: "evil"
    }

    property Outer outer: Outer {}
    property Outer evil: Evil {}

    property QtObject notShadowable: QtObject {
        objectName: "notShadowable"
    }

    function getInnerShadowable() {
        notShadowable = outer.inner.shadowable;
    }

    function setInnerShadowable() {
        outer.inner.shadowable = self;
    }

    function turnEvil() {
        outer = evil;
    }
}
