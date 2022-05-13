// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLSTRINGCONVERTERS_P_H
#define QQMLSTRINGCONVERTERS_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>

#include <private/qtqmlglobal_p.h>

QT_BEGIN_NAMESPACE

class QPointF;
class QSizeF;
class QRectF;
class QString;
class QByteArray;

namespace QQmlStringConverters
{
    Q_QML_PRIVATE_EXPORT QVariant variantFromString(const QString &, QMetaType preferredType, bool *ok = nullptr);

    Q_QML_PRIVATE_EXPORT QVariant colorFromString(const QString &, bool *ok = nullptr);
    Q_QML_PRIVATE_EXPORT unsigned rgbaFromString(const QString &, bool *ok = nullptr);

#if QT_CONFIG(datestring)
    Q_QML_PRIVATE_EXPORT QDate dateFromString(const QString &, bool *ok = nullptr);
    Q_QML_PRIVATE_EXPORT QTime timeFromString(const QString &, bool *ok = nullptr);
    Q_QML_PRIVATE_EXPORT QDateTime dateTimeFromString(const QString &, bool *ok = nullptr);
#endif
    Q_QML_PRIVATE_EXPORT QPointF pointFFromString(const QString &, bool *ok = nullptr);
    Q_QML_PRIVATE_EXPORT QSizeF sizeFFromString(const QString &, bool *ok = nullptr);
    Q_QML_PRIVATE_EXPORT QRectF rectFFromString(const QString &, bool *ok = nullptr);
}

QT_END_NAMESPACE

#endif // QQMLSTRINGCONVERTERS_P_H
