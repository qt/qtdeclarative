import TestTypes

Person {
    property Person other: Person { id: oo }
    barzles: oo.barzles
    cousins: oo.cousins
    property int l: oo.barzles.length
    property int m: oo.cousins.length

    property list<Person> others: cousins
}
