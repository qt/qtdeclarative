// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKIOSSTYLE_P_H
#define QQUICKIOSSTYLE_P_H

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

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickIOSStyle : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url CONSTANT FINAL)
    QML_NAMED_ELEMENT(IOS)
    QML_SINGLETON
    QML_ADDED_IN_VERSION(6, 5)

public:
    explicit QQuickIOSStyle(QObject *parent = nullptr);

    static QUrl url();
};

QT_END_NAMESPACE

#endif // QQUICKIOSSTYLE_P_H
