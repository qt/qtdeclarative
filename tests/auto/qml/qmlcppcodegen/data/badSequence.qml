import TestTypes

Person {
    id: self
    property Person other: Person { id: oo }
    barzles: oo.barzles
    cousins: oo.cousins
    property int l: oo.barzles.length
    property int m: oo.cousins.length

    property list<Person> others: cousins

    property Person mom: Person {
        id: mm
        cousins: self.others
    }
    property list<Person> momsCousins: mm.cousins

    property Person dad: Person {
        id: dd
        cousins: oo
    }
    property list<Person> dadsCousins: dd.cousins
}
