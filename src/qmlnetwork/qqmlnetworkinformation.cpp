// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmlnetworkinformation_p.h"

QT_BEGIN_NAMESPACE

QNetworkInformation *QQmlNetworkInformation::create(QQmlEngine *, QJSEngine *)
{
    static QNetworkInformation *s_singletonInstance = []() {
        QNetworkInformation::loadDefaultBackend();
        QNetworkInformation *singletonInstance = QNetworkInformation::instance();

        Q_ASSERT(singletonInstance);
        QJSEngine::setObjectOwnership(singletonInstance, QJSEngine::CppOwnership);
        return singletonInstance;
    }();

    return s_singletonInstance;
}

QT_END_NAMESPACE

#include "moc_qqmlnetworkinformation_p.cpp"
