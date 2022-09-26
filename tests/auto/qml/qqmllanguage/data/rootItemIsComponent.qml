import QtQuick 2.0

Component {
    id: evilTopLevelComponent

    Item {
        component EvilICComponent: Component {}

        EvilComponentType {
            id: evilTopLevelComponentFromAnotherFile

            Item {}
        }
    }
}
