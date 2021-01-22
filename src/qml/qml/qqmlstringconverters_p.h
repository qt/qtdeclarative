/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
    Q_QML_PRIVATE_EXPORT QVariant variantFromString(const QString &);
    Q_QML_PRIVATE_EXPORT QVariant variantFromString(const QString &, int preferredType, bool *ok = nullptr);

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

    Q_QML_PRIVATE_EXPORT bool createFromString(int, const QString &, void *, size_t);
}

QT_END_NAMESPACE

#endif // QQMLSTRINGCONVERTERS_P_H
