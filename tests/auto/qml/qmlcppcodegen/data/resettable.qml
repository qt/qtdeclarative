pragma Strict
import QtQml
import TestTypes

Resettable {
    id: self
    value: 999

    property Resettable shadowing: Resettable {
        property var shadowable: undefined
    }

    function doReset() { self.value = undefined }
    function doReset2() { self.value = shadowing.shadowable }
}
