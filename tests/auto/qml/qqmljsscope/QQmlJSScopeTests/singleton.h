// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SINGLETON_H
#define SINGLETON_H

#include <QtCore/qobject.h>
#include <QtGui/qfont.h>
#include <qqml.h>

class TestApplication : public QObject
{
    Q_OBJECT
    QML_SINGLETON

public:
    TestApplication(QObject *parent = nullptr) : QObject(parent) { }

    Q_INVOKABLE QFont createDummyFont() const;
};

#endif // SINGLETON_H
