pragma ComponentBehavior: Bound
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

    readonly property Component fooComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + foo.objectName }
                property string probeBinding: "" + foo.objectName
                property string probeScript: { return "" + foo.objectName }
            }
        }
    }

    readonly property Component fooContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + foo.objectName }
                property string probeBinding: "" + foo.objectName
                property string probeScript: { return "" + foo.objectName }
            }
        }
    }

    readonly property Component fooDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + foo.objectName }
            }
        }
    }

    readonly property Component fooIdComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject idHolder: QtObject { id: foo; objectName: "id" }
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + foo.objectName }
                property string probeBinding: "" + foo.objectName
                property string probeScript: { return "" + foo.objectName }
            }
        }
    }

    readonly property Component fooScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + foo.objectName }
                property string probeBinding: "" + foo.objectName
                property string probeScript: { return "" + foo.objectName }
            }
        }
    }

    readonly property Component rectComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + rect.objectName }
                property string probeBinding: "" + rect.objectName
                property string probeScript: { return "" + rect.objectName }
            }
        }
    }

    readonly property Component rectContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + rect.objectName }
                property string probeBinding: "" + rect.objectName
                property string probeScript: { return "" + rect.objectName }
            }
        }
    }

    readonly property Component rectDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + rect.objectName }
            }
        }
    }

    readonly property Component rectIdComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject idHolder: QtObject { id: rect; objectName: "id" }
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + rect.objectName }
                property string probeBinding: "" + rect.objectName
                property string probeScript: { return "" + rect.objectName }
            }
        }
    }

    readonly property Component rectScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + rect.objectName }
                property string probeBinding: "" + rect.objectName
                property string probeScript: { return "" + rect.objectName }
            }
        }
    }
}
