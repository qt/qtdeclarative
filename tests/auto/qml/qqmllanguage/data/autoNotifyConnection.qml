import qqmllanguage.Test 1.0
MyQmlObject {
    property bool receivedNotify : false
    onPropertyWithNotifyChanged: { receivedNotify = true; }
}

