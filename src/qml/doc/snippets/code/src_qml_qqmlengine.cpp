// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
class MySingleton : public QObject {
    Q_OBJECT

    // Register as default constructed singleton.
    QML_ELEMENT
    QML_SINGLETON

    static int typeId;
    // ...
};
//! [0]

/*
//! [1]
    MySingleton::typeId = qmlTypeId(...);
//! [1]
*/

void wrapper2() {
//! [2]
    // Retrieve as QObject*
    QQmlEngine engine;
    MySingleton* instance = engine.singletonInstance<MySingleton*>(MySingleton::typeId);
//! [2]
}

/*
//! [3]
    // Register with QJSValue callback
    int typeId = qmlRegisterSingletonType(...);
//! [3]
*/

void wrapper4(int typeId) {
//! [4]
    // Retrieve as QJSValue
    QQmlEngine engine;
    QJSValue instance = engine.singletonInstance<QJSValue>(typeId);
//! [4]
}

void wrapper5() {
///! [5]
    QQmlEngine engine;
    MySingleton *singleton = engine.singletonInstance<MySingleton *>("mymodule", "MySingleton");
///! [5]
}
