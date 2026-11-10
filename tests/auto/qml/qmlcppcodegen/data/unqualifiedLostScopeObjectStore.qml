import QtQml

// For tst_QmlCppCodegen::unqualifiedLostScopeObjectStore. The test destroys the scope object, but
// the context object and the function survive. A store to a name the scope object provides has to
// throw a ReferenceError then. It must neither store to the context object, which has the name
// "shared", too, nor to the global object.
QtObject {
    readonly property Component inner: Component {
        QtObject {
            objectName: "context object"
            property int shared
            property var storedTarget
            property var storedShared

            Component.onCompleted: {
                storedTarget = child.probeStoreTarget
                storedShared = child.probeStoreShared
            }

            readonly property QtObject child: QtObject {
                objectName: "scope object"
                property int target
                property int shared

                function probeStoreTarget(unused) {
                    target = 5
                    return "stored"
                }
                function probeStoreShared(unused) {
                    shared = 5
                    return "stored"
                }
            }
        }
    }

    function makeInner(parent: QtObject): QtObject {
        return inner.createObject(parent)
    }

    function callStored(context: QtObject, name: string): string {
        try {
            context[name](0)
            return "stored"
        } catch (e) {
            return "<" + e.name + ">"
        }
    }
}
