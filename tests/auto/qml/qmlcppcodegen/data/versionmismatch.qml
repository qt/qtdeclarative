import QtQml
import TestTypes 1.0

QtObject {
    property int inaccessible: 4
    property CppBaseClass a: CppBaseClass {
        property int b: inaccessible + 1
    }
    property int c: a.b
}
