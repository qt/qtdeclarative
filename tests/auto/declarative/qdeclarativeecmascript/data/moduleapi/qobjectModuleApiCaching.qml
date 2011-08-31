import QtQuick 2.0

import Qt.test 1.0 as QtTest                                      // module API installed into existing uri
import Qt.test.qobjectApiParented 1.0 as QtTestParentedQObjectApi // qobject (with parent) module API installed into a new uri

QtObject {
    property int existingUriTest: QtTest.qobjectTestProperty
    property int qobjectParentedTest: QtTestParentedQObjectApi.qobjectTestProperty
}

