// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TESTDRIVER_H
#define TESTDRIVER_H
#include <QtCore>
#include <QtQml>

class TestDriver : public QObject
{
    Q_OBJECT
public:
    TestDriver(const QUrl &componentUrl, int maxIter = -1);
public slots:
    void setUp();
    void tearDown();
private:
    QUrl testFile;
    QQmlEngine* e;
    QQmlComponent* c;
    signed long int i;
};

class TestType : public QObject { Q_OBJECT };
class TestType2 : public QObject { Q_OBJECT };
class TestType3 : public QObject { Q_OBJECT };
#endif
