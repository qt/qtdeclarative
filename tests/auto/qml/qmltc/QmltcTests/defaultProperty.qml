import QtQml 2.0

DefaultPropertySingleChild {
    property string hello: "Hello from parent"

    DefaultPropertyManyChildren {
        property string hello: "Hello from parent.child"

        QtObject {
            property string hello: "Hello from parent.child.children[0]"
        }

        QtObject {
            property string hello: "Hello from parent.child.children[1]"
        }
    }
}
