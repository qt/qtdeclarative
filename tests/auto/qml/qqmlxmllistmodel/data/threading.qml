import QtQml.XmlListModel

XmlListModel {
    query: "/data/item"
    XmlListModelRole { name: "name"; elementName: "name" }
    XmlListModelRole { name: "age"; elementName: "age" }
    XmlListModelRole { name: "sport"; elementName: "sport" }
}
