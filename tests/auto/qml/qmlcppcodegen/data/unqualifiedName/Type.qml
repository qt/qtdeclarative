import QtQml
import TestTypes
import ShadowTypes


// For tst_QmlCppCodegen::unqualifiedName. Each component places a name as id in the
// component context, and on the scope object or the context object, or not at all. In the
// files with "Root" in their names, the root object is the enclosing context object, and it
// has all the names. In the files with "Swapped" in their names, the objects holding the
// names have the other kind of member for each name.
QtObject {
    readonly property Component infinityComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + Infinity }
                property string probeBinding: "" + Infinity
                property string probeScript: { return "" + Infinity }
            }
        }
    }

    readonly property Component infinityContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + Infinity }
                property string probeBinding: "" + Infinity
                property string probeScript: { return "" + Infinity }
            }
        }
    }

    readonly property Component infinityDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + Infinity }
            }
        }
    }

    readonly property Component infinityScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + Infinity }
                property string probeBinding: "" + Infinity
                property string probeScript: { return "" + Infinity }
            }
        }
    }

    readonly property Component mathComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + Math.max(1, 2) }
                property string probeBinding: "" + Math.max(1, 2)
                property string probeScript: { return "" + Math.max(1, 2) }
            }
        }
    }

    readonly property Component mathContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + Math.max(1, 2) }
                property string probeBinding: "" + Math.max(1, 2)
                property string probeScript: { return "" + Math.max(1, 2) }
            }
        }
    }

    readonly property Component mathDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + Math.max(1, 2) }
            }
        }
    }

    readonly property Component mathScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + Math.max(1, 2) }
                property string probeBinding: "" + Math.max(1, 2)
                property string probeScript: { return "" + Math.max(1, 2) }
            }
        }
    }

    readonly property Component plughComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + Plugh.objectName }
                property string probeBinding: "" + Plugh.objectName
                property string probeScript: { return "" + Plugh.objectName }
            }
        }
    }

    readonly property Component plughContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + Plugh.objectName }
                property string probeBinding: "" + Plugh.objectName
                property string probeScript: { return "" + Plugh.objectName }
            }
        }
    }

    readonly property Component plughDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + Plugh.objectName }
            }
        }
    }

    readonly property Component plughScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + Plugh.objectName }
                property string probeBinding: "" + Plugh.objectName
                property string probeScript: { return "" + Plugh.objectName }
            }
        }
    }
}
