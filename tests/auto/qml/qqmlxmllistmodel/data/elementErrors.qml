import QtQml.XmlListModel

XmlListModel {
    source: "model.xml"
    query: "/Pets/Pet"
    XmlListModelRole { name: "name"; elementName: "/name" }         // starts with '/'
    XmlListModelRole { name: "age"; elementName: "age/" }           // ends with '/'
    XmlListModelRole { name: "type"; elementName: "some//element" } // contains "//"
}
