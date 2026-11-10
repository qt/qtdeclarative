import QtQml
import TestTypes
import ShadowTypes


// For tst_QmlCppCodegen::unqualifiedName. Each component places a name as id in the
// component context, and on the scope object or the context object, or not at all. In the
// files with "Root" in their names, the root object is the enclosing context object, and it
// has all the names. In the files with "Swapped" in their names, the objects holding the
// names have the other kind of member for each name.
SwappedNameHolder {
    objectName: "enclosing context object"

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
}
