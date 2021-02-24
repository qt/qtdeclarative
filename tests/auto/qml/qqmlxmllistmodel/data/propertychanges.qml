import QtQml.XmlListModel

XmlListModel {
    source: "model.xml"
    query: "/Pets/Pet"
    XmlListModelRole { objectName: "role"; name: "name"; elementName: "name" }
    XmlListModelRole { name: "type"; elementName: "type" }
    XmlListModelRole { name: "age"; elementName: "age" }
    XmlListModelRole { name: "size"; elementName: "size" }
}
