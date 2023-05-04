pragma Strict
import QtQml
import TestTypes

Person {
    property list<var> varlist: [1, "foo", this, undefined, true]
    property list<QtObject> objlist: [this, null, this]

    function sendSignals() {
        variantListHappened(varlist);
        objectListHappened(objlist);
    }

    property int happening: 0

    onObjectListHappened:  (objects)  => { happening += objects.length  }
    onVariantListHappened: (variants) => { happening += variants.length }
}
