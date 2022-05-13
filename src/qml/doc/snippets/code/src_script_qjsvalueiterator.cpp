// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QJSValue object;
...
QJSValueIterator it(object);
while (it.hasNext()) {
    it.next();
    qDebug() << it.name() << ": " << it.value().toString();
}
//! [0]


//! [1]
QJSValue obj = ...; // the object to iterate over
while (obj.isObject()) {
    QJSValueIterator it(obj);
    while (it.hasNext()) {
        it.next();
        qDebug() << it.name();
    }
    obj = obj.prototype();
}
//! [1]

