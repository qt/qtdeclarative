import QtQml 2.0
import qqmllanguage.Test 1.0
LazyDeferredSubObject {
    subObject: QtObject { objectName: 'default' }
    objectName: subObject.objectName
}
