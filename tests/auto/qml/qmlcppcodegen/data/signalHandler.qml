import QtQml

QtObject {
    signal foo
    signal bar(baz: string)

    onFoo: ()=> console.log("foo")
    onBar: (baz)=> console.log(baz)
}
