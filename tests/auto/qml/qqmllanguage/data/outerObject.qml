import QtQml
import "testlib.js" as TestLib

QtObject {
    id: self

    property Component delegate

    property ObjectWithProperty objectWithProperty: ObjectWithProperty {}

    property InnerObject innerObject: null

    property var logger: TestLib.return_a_thing()

    function doInstantiate() {
        innerObject = delegate.createObject(self, {objectWithProperty: objectWithProperty})
    }
}
