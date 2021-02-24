import QtQml.XmlListModel

XmlListModel {
    source: "nestedElements.xml"
    query: "/Pets/Pet"

    XmlListModelRole { name: "name"; elementName: "name" }
    XmlListModelRole { name: "type"; elementName: "some/other/useful/tags/type" }
    XmlListModelRole { name: "age"; elementName: "some/other/age"; attributeName: "value" }
    XmlListModelRole { name: "size"; elementName: "some/size" }
}
