import QtQml 2.0
QtObject {
    property SelfReference self
    signal blah(selfParam: SelfReference)
    function returnSelf() : SelfReference {
        return this;
    }
}
