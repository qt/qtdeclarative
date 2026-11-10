pragma ComponentBehavior: Bound
import QtQml
import TestTypes

// For tst_QmlCppCodegen::unqualifiedLostEnclosingContextObject. The root object is the enclosing
// context object of the objects the inner component creates. It has the names, and the inner
// component is bound, so it searches them. The test destroys the root object, but the inner
// objects survive. Their code then has to throw a ReferenceError for a name the root object would
// have provided. It must neither find the global object's member of the same name nor continue
// with other contexts.
SwappedNameHolder {
    objectName: "enclosing context object"

    readonly property Component inner: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeName(unused) {
                    return "" + foo().objectName
                }
                function probeGlobal(unused) {
                    return "" + console().debug("text")
                }
            }
        }
    }

    function makeInner(parent: QtObject): QtObject {
        return inner.createObject(parent)
    }
}
