import qt.test 1.0
import QtQml
QtObject {
    AttachedRequiredProperty.onIndexChanged: {
        var s = "this is just to trigger the attachment"
    }
} // error: we don't support required attached properties
