import Test
import QtQml

Foo {
    a.a: 12
    b.a: 13
    fooProperty: [a, b, Component]
}
