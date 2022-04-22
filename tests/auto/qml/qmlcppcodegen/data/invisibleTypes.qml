import QtQml
import TestTypes

QtObject {
    property var singleton: SingletonModel
    property var attached: AttachedObject
    property var metaobject: DerivedFromInvisible
}
