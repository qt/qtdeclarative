import QtQuick
import QmltcQProcessTests

Item {
    // Illegal cases:
    UncreatableType {}
    SingletonThing {}
    SingletonType {}

    component A: SingletonThing {}
    component AA: A {}
    component AAA: AA {}
    AAA {}

    component B: SingletonType {}
    component BB: B {}
    component BBB: BB {}
    BBB {}

    // Legal cases, where qmltc should not crash
    property SingletonThing myQmlSingleton
    property SingletonType myCppSingleton
    NotSingletonType {} // ok because a non composite type inheriting from a singleton does not become a singleton!
}
