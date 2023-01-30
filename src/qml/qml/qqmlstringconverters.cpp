// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlstringconverters_p.h"
#include <private/qqmlglobal_p.h>

#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtCore/qsize.h>
#include <QtCore/qvariant.h>
#include <QtCore/qdatetime.h>

QT_BEGIN_NAMESPACE

QVariant QQmlStringConverters::variantFromString(const QString &s, QMetaType preferredType, bool *ok)
{
    switch (preferredType.id()) {
    case QMetaType::Int:
        return QVariant(int(qRound(s.toDouble(ok))));
    case QMetaType::UInt:
        return QVariant(uint(qRound(s.toDouble(ok))));
#if QT_CONFIG(datestring)
    case QMetaType::QDate:
        return QVariant::fromValue(dateFromString(s, ok));
    case QMetaType::QTime:
        return QVariant::fromValue(timeFromString(s, ok));
    case QMetaType::QDateTime:
        return QVariant::fromValue(dateTimeFromString(s, ok));
#endif // datestring
    case QMetaType::QPointF:
        return QVariant::fromValue(pointFFromString(s, ok));
    case QMetaType::QPoint:
        return QVariant::fromValue(pointFFromString(s, ok).toPoint());
    case QMetaType::QSizeF:
        return QVariant::fromValue(sizeFFromString(s, ok));
    case QMetaType::QSize:
        return QVariant::fromValue(sizeFFromString(s, ok).toSize());
    case QMetaType::QRectF:
        return QVariant::fromValue(rectFFromString(s, ok));
    case QMetaType::QRect:
        return QVariant::fromValue(rectFFromString(s, ok).toRect());
    default: {
        const QVariant ret = QQmlValueTypeProvider::createValueType(s, preferredType);
        if (ret.isValid()) {
            if (ok)
                *ok = true;
            return ret;
        }
        if (ok)
            *ok = false;
        return QVariant();
    }
    }
}

QVariant QQmlStringConverters::colorFromString(const QString &s, bool *ok)
{
    return QQml_colorProvider()->colorFromString(s, ok);
}

unsigned QQmlStringConverters::rgbaFromString(const QString &s, bool *ok)
{
    return QQml_colorProvider()->rgbaFromString(s, ok);
}

#if QT_CONFIG(datestring)
QDate QQmlStringConverters::dateFromString(const QString &s, bool *ok)
{
    QDate d = QDate::fromString(s, Qt::ISODate);
    if (ok) *ok =  d.isValid();
    return d;
}

QTime QQmlStringConverters::timeFromString(const QString &s, bool *ok)
{
    QTime t = QTime::fromString(s, Qt::ISODate);
    if (ok) *ok = t.isValid();
    return t;
}

QDateTime QQmlStringConverters::dateTimeFromString(const QString &s, bool *ok)
{
    QDateTime d = QDateTime::fromString(s, Qt::ISODate);
    if (ok) *ok =  d.isValid();
    return d;
}
#endif // datestring

//expects input of "x,y"
QPointF QQmlStringConverters::pointFFromString(const QString &s, bool *ok)
{
    if (s.count(QLatin1Char(',')) != 1) {
        if (ok)
            *ok = false;
        return QPointF();
    }

    bool xGood, yGood;
    int index = s.indexOf(QLatin1Char(','));
    qreal xCoord = QStringView{s}.left(index).toDouble(&xGood);
    qreal yCoord = QStringView{s}.mid(index+1).toDouble(&yGood);
    if (!xGood || !yGood) {
        if (ok)
            *ok = false;
        return QPointF();
    }

    if (ok)
        *ok = true;
    return QPointF(xCoord, yCoord);
}

//expects input of "widthxheight"
QSizeF QQmlStringConverters::sizeFFromString(const QString &s, bool *ok)
{
    if (s.count(QLatin1Char('x')) != 1) {
        if (ok)
            *ok = false;
        return QSizeF();
    }

    bool wGood, hGood;
    int index = s.indexOf(QLatin1Char('x'));
    qreal width = QStringView{s}.left(index).toDouble(&wGood);
    qreal height = QStringView{s}.mid(index+1).toDouble(&hGood);
    if (!wGood || !hGood) {
        if (ok)
            *ok = false;
        return QSizeF();
    }

    if (ok)
        *ok = true;
    return QSizeF(width, height);
}

//expects input of "x,y,widthxheight" //### use space instead of second comma?
QRectF QQmlStringConverters::rectFFromString(const QString &s, bool *ok)
{
    if (s.count(QLatin1Char(',')) != 2 || s.count(QLatin1Char('x')) != 1) {
        if (ok)
            *ok = false;
        return QRectF();
    }

    bool xGood, yGood, wGood, hGood;
    int index = s.indexOf(QLatin1Char(','));
    qreal x = QStringView{s}.left(index).toDouble(&xGood);
    int index2 = s.indexOf(QLatin1Char(','), index+1);
    qreal y = QStringView{s}.mid(index+1, index2-index-1).toDouble(&yGood);
    index = s.indexOf(QLatin1Char('x'), index2+1);
    qreal width = QStringView{s}.mid(index2+1, index-index2-1).toDouble(&wGood);
    qreal height = QStringView{s}.mid(index+1).toDouble(&hGood);
    if (!xGood || !yGood || !wGood || !hGood) {
        if (ok)
            *ok = false;
        return QRectF();
    }

    if (ok)
        *ok = true;
    return QRectF(x, y, width, height);
}

QT_END_NAMESPACE
