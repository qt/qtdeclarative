import QtQuick 2.0

Item {
    id: self
    property font myFont
    property int notMyFont

    property var object

    function callWithFont() {
        object.method_gadget(myFont) // should be fine
    }
    function callWithInt() {
        object.method_gadget(123)
    }
    function callWithInt2() {
        object.method_gadget(notMyFont)
    }
    function callWithNull() {
        object.method_gadget(null)
    }
    function callWithAllowedNull() {
        object.method_qobject(null)
    }
}
