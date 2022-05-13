// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QJSEngine myEngine;
QJSValue myObject = myEngine.newObject();
QJSValue myOtherObject = myEngine.newObject();
myObject.setProperty("myChild", myOtherObject);
myObject.setProperty("name", "John Doe");
//! [0]


//! [1]
QJSEngine engine;
engine.evaluate("function fullName() { return this.firstName + ' ' + this.lastName; }");
engine.evaluate("somePerson = { firstName: 'John', lastName: 'Doe' }");

QJSValue global = engine.globalObject();
QJSValue fullName = global.property("fullName");
QJSValue who = global.property("somePerson");
qDebug() << fullName.call(who).toString(); // "John Doe"

engine.evaluate("function cube(x) { return x * x * x; }");
QJSValue cube = global.property("cube");
QJSValueList args;
args << 3;
qDebug() << cube.call(QJSValue(), args).toNumber(); // 27
//! [1]
