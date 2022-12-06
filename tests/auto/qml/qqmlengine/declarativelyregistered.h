// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef DECLARATIVELYREGISTERED_H
#define DECLARATIVELYREGISTERED_H

#include <QtCore/qobject.h>
#include <QtQml/qqmlregistration.h>

class PurelyDeclarativeSingleton : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
public:
    PurelyDeclarativeSingleton(QObject *parent = nullptr);
};


#endif
