pragma ComponentBehavior: Bound
pragma ValueTypeBehavior: Addressable
import QtQml
import TestTypes


// For tst_QmlCppCodegen::unqualifiedName. Each component places a name as id in the
// component context, and on the scope object or the context object, or not at all. In the
// files with "Root" in their names, the root object is the enclosing context object, and it
// has all the names. In the files with "Swapped" in their names, the objects holding the
// names have the other kind of member for each name.
NameHolder {
    objectName: "enclosing context object"

    readonly property Component rectComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + rect }
                property string probeBinding: "" + rect
                property string probeScript: { return "" + rect }
            }
        }
    }
}
