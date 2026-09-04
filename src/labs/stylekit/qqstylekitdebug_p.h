// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITDEBUG_P_H
#define QQSTYLEKITDEBUG_P_H

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

#include <QtQuick/qquickitem.h>

#include "qqstylekitpropertypath_p.h"
#include "qqstylekitcontrolproperties_p.h"

QT_BEGIN_NAMESPACE

class QQStyleKitDebug: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *control READ control WRITE setControl NOTIFY controlChanged FINAL)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged FINAL)
    QML_NAMED_ELEMENT(StyleKitDebug)

public:
    QQuickItem *control() const;
    void setControl(QQuickItem *item);

    QString filter() const;
    void setFilter(const QString &filter);

    Q_INVOKABLE static bool insideControl(const QObject *child);

signals:
    void controlChanged();
    void filterChanged();

private:
    static const QQStyleKitPropertyGroup *groupBeingRead;
    static QPointer<QQuickItem> m_item;
    static QString m_filter;
    static int m_outputCount;

private:
    static inline bool enabled() { return m_item != nullptr; }
    static void notifyPropertyNotResolved(const QQStyleKitPropertyPath property);
    static void notifyPropertyRead(
        const QQStyleKitPropertyPath property,
        const QQStyleKitControlProperties *resolvedControl,
        const QQSK::State state,
        const QVariant &value);
    static void notifyPropertyWrite(
        const QQStyleKitPropertyGroup *group,
        const QQSK::Property property,
        const QQStyleKitControlProperties *storage,
        const QQSK::State state,
        const PropertyStorageId key,
        const QVariant &value);
    static void trace(
        const QQStyleKitPropertyPath property,
        const QQStyleKitControlProperties *resolvedControl,
        const QQSK::State state,
        const PropertyStorageId key);

    template <typename EnumType>
    static QString enumToString(EnumType enumValue);
    static QString objectName(const QObject *obj);
    static QString stateToString(const QQSK::State state);
    static QString styleReaderToString(const QQStyleKitReader *reader);
    static QString controlToString(const QQStyleKitControlProperties *control);
    static QString objectPath(const QQStyleKitControlProperties *properties, QObject *from);
    static QString propertyPath(const QQStyleKitPropertyGroup *group, const QQStyleKitPropertyPath property);

    friend class QQStyleKitPropertyResolver;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITDEBUG_P_H
