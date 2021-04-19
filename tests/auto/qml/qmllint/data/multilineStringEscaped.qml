import QtQml

QtObject {
    property string test: "Foo\nmultiline\nString"
    property string template: `Foo
multiline
string`
}
