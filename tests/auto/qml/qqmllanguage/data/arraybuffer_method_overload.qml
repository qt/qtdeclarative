import QtQuick 2.7
import qqmllanguage.Test 1.0

MyArrayBufferTestClass {
    property bool ok: false
    Component.onCompleted: ok = byteArrayMethod_Overloaded(new ArrayBuffer());
}
