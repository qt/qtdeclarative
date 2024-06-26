// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef DECLARATIVELYREGISTERED_LANGUAGE_H
#define DECLARATIVELYREGISTERED_LANGUAGE_H

#include <QtCore/qobject.h>
#include <QtQml/qqmlregistration.h>

class PurelyDeclarativeSingleton : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
public:
    PurelyDeclarativeSingleton();
};


#endif
