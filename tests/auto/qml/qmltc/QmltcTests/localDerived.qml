// NB: special broken case where imported base type has Component.onCompleted
// handler which sets up JavaScript context but in the wrong way?
LocalWithOnCompleted {
    property int p1: 323
    property int p2: p1 + 10
}
