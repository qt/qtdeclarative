// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

namespace DocWrapper0 {
//! [0]
class FruitBasket : QObject {
    Q_OBJECT
    QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_APPEND
    Q_PROPERTY(QQmlListProperty<Fruit> fruit READ fruit)

    public:
    // ...
    QQmlListProperty<Fruit> fruit();
    // ...
};
//! [0]
}

namespace DocWrapper1 {
//! [1]
class FruitBasket : QObject {
    Q_OBJECT
    QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_REPLACE_IF_NOT_DEFAULT
    Q_PROPERTY(QQmlListProperty<Fruit> fruit READ fruit)

    public:
    // ...
    QQmlListProperty<Fruit> fruit();
    // ...
};
//! [1]
}

namespace DocWrapper2 {
//! [2]
class FruitBasket : QObject {
    Q_OBJECT
    QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_REPLACE
    Q_PROPERTY(QQmlListProperty<Fruit> fruit READ fruit)

    public:
    // ...
    QQmlListProperty<Fruit> fruit();
    // ...
};
//! [2]
}
