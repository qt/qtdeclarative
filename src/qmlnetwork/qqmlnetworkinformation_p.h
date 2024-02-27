// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLNETWORKINFORMATION_P_H
#define QQMLNETWORKINFORMATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QQmlEngine>
#include <QJSEngine>

#include <QtNetwork/qnetworkinformation.h>
#include <private/qtqmlnetworkexports_p.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

struct Q_QMLNETWORK_PRIVATE_EXPORT QQmlNetworkInformation
{
    Q_GADGET
    QML_FOREIGN(QNetworkInformation)
    QML_NAMED_ELEMENT(NetworkInformation)
    QML_ADDED_IN_VERSION(6, 7)
    QML_SINGLETON

public:
    static QNetworkInformation *create(QQmlEngine *, QJSEngine *);
};

QT_END_NAMESPACE

#endif
