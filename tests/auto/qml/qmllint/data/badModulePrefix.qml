import QtQml
import TestTypes as T

QtObject {
    id: self
    property string baz: self.T.CppSingleton.objectName
}
