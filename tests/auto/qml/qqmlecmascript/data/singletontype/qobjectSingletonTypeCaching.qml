import QtQuick 2.0

import Qt.test 1.0 as QtTest                                      // singleton Type installed into existing uri
import Qt.test.qobjectApiParented 1.0 as QtTestParentedQObjectApi // qobject (with parent) singleton Type installed into a new uri

QtObject {
    property int existingUriTest: QtTest.QObject.qobjectTestProperty
    property int qobjectParentedTest: QtTestParentedQObjectApi.QObject.qobjectTestProperty
}

