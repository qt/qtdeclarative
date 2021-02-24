import QtQml.XmlListModel

XmlListModel {
    source: "model.xml"
    query: "/Pets/Pet"
    XmlListModelRole { name: "name"; elementName: "name" }
    XmlListModelRole { name: "name"; elementName: "type" }
}
