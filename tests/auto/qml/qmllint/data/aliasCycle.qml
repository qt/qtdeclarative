import QtQml

QtObject {
    id: self
    property alias a: self.b
    property alias b: self.a
}
