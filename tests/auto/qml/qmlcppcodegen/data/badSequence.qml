import TestTypes

Person {
    property Person other: Person { id: oo }
    barzles: oo.barzles
    property int l: oo.barzles.length
}
