import QtQml

QtObject {
    id: self
    property real foo: 12.3
    property alias a: self.foo
    property string t: Math.round(self.a)
}
