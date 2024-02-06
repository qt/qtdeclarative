pragma Strict
import QtQml
import TestTypes

Resettable {
    id: self
    value: 999

    property double notResettable: 10
    property double notResettable2: { return undefined }

    property Resettable shadowing: Resettable {
        property var shadowable: undefined
    }

    function doReset() { self.value = undefined }
    function doReset2() { self.value = shadowing.shadowable }
    function doNotReset() { self.notResettable = undefined }

    signal aaa()
    signal bbb()
    onAaa: objectName = self.bbb()
}
