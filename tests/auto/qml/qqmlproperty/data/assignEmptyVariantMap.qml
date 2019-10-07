import QtQuick 2.0

Item {
    required property QtObject o
    Component.onCompleted: { o.variantMap = {}; }
}
