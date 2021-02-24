import QtQuick 2.0
import QtQml.XmlListModel

XmlListModel {
    id: model
    XmlListModelRole {}
    Component.onCompleted: {
        model.roles = 0
    }
}
