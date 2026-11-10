import QtQml
import TestTypes


// For tst_QmlCppCodegen::unqualifiedName. Each component places a name as id in the
// component context, and on the scope object or the context object, or not at all. In the
// files with "Root" in their names, the root object is the enclosing context object, and it
// has all the names. In the files with "Swapped" in their names, the objects holding the
// names have the other kind of member for each name.
QtObject {
    readonly property QtObject fooHolder: QtObject { id: foo; objectName: "enclosing id" }
    readonly property QtObject rectHolder: QtObject { id: rect; objectName: "enclosing id" }

    readonly property Component fooScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: SwappedNameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + foo().objectName }
                property string probeBinding: "" + foo().objectName
                property string probeScript: { return "" + foo().objectName }
            }
        }
    }

    readonly property Component rectScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: SwappedNameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + rect().objectName }
                property string probeBinding: "" + rect().objectName
                property string probeScript: { return "" + rect().objectName }
            }
        }
    }

    readonly property Component fooContextComponent: Component {
        SwappedNameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + foo().objectName }
                property string probeBinding: "" + foo().objectName
                property string probeScript: { return "" + foo().objectName }
            }
        }
    }

    readonly property Component rectContextComponent: Component {
        SwappedNameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + rect().objectName }
                property string probeBinding: "" + rect().objectName
                property string probeScript: { return "" + rect().objectName }
            }
        }
    }
}
