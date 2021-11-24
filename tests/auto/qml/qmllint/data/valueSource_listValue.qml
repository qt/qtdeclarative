import QtQuick 2.0

Item {
    property list<QtObject> objs: [ QtObject {} ]
    NumberAnimation on objs { // Note: poor example but it should error anyway
    }
}
