import QtQml

QtObject {
    id: self

    property Component delegate

    property ObjectWithProperty objectWithProperty: ObjectWithProperty {}

    property InnerObject innerObject: null

    function doInstantiate() {
        innerObject = delegate.createObject(self, {objectWithProperty: objectWithProperty})
    }
}
