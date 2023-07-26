pragma Strict
import QtQml
import TestTypes

QtObject {
    objectName: people[0].getName()
    property list<Person> people: [
        Person {
            name: "no one"
        }
    ]

    function boom() : string {
        return people[1].getName()
    }
}
