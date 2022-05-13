// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLTYPENOTAVAILABLE_H
#define QQMLTYPENOTAVAILABLE_H

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

#include <qqml.h>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QQmlTypeNotAvailable : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(TypeNotAvailable)
    QML_ADDED_IN_VERSION(2, 15)
    QML_UNCREATABLE("Type not available.")
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQmlTypeNotAvailable)

#endif // QQMLTYPENOTAVAILABLE_H
