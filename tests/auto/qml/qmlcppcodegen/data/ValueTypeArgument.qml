pragma Strict

import QtQml
import TestTypes

QtObject {
    id: root
    property largeValueType l

    function callee(l: largeValueType) : void {
        console.log("Reading l.i=" + l.i)
    }

    function caller() : void {
        callee(root.l)
    }

    Component.onCompleted: {
        caller()
    }
}
