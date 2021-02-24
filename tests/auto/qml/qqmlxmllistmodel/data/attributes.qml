import QtQml.XmlListModel

XmlListModel {
    source: "attributes.xml"
    query: "/Pets/Pet"

    XmlListModelRole { name: "name"; elementName: ""; attributeName: "name" }
    XmlListModelRole { name: "type"; elementName: "info"; attributeName: "type" }
    XmlListModelRole { name: "age"; elementName: "info"; attributeName: "age" }
    XmlListModelRole { name: "size"; elementName: "info"; attributeName: "size" }
}
