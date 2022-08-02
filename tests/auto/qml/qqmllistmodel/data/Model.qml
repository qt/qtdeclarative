import QtQml.Models

ListModel {
    enum Choose { Foo, Bar, Baz }

    ListElement { choose: Model.Choose.Foo }
    ListElement { choose: Model.Choose.Bar }
    ListElement { choose: Model.Choose.Baz }
}
