import QtQml.XmlListModel

XmlListModel {
    source: "recipes.xml"
    query: "/recipes/recipe"
    XmlListModelRole { name: "title"; elementName: ""; attributeName: "title" }
    XmlListModelRole { name: "picture"; elementName: "picture" }
    XmlListModelRole { name: "ingredients"; elementName: "ingredients" }
    XmlListModelRole { name: "preparation"; elementName: "method" }
}
