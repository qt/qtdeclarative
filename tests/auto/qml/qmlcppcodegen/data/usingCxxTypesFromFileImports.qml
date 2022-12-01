pragma Strict
import QtQml

QtObject {
    objectName: a.objectName + " " + b.objectName
    property CxxTypeFromDir a: CxxTypeFromDir { id: a }
    property CxxTypeFromImplicit b: CxxTypeFromImplicit { id: b }

}
