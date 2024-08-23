import QtQml

QtObject {
    property AnonymousComposite a: AnonymousComposite { id: aa }
    property int b: aa.theInner.a
}
