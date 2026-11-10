pragma Strict
pragma ComponentBehavior: Bound
import QtQml
import TestTypes

// The inline component is instantiated in the context of the root object. That context belongs to
// the same document, but it does not lexically enclose the inline component. Its id and its
// context object are only searched among the remaining contexts, after the scope object and the
// global object.
NameHolder {
    objectName: "instantiation site"
    readonly property QtObject fooHolder: QtObject { id: foo; objectName: "instantiation site id" }

    component Inner: QtObject {
        readonly property QtObject child: NameHolder {
            objectName: "scope object"
            function probeId(): string { return "" + foo.objectName }
        }
        function probeGlobal(): string { return "" + Math.max(1, 2) }
    }

    readonly property Inner inner: Inner {}
}
