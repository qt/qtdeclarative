import QtQuick

Item {
    property string qtTrIdNoopTest: QT_TRID_NOOP("Hello")
    property string qsTrIdTest: qsTrId("hello_id")
}
