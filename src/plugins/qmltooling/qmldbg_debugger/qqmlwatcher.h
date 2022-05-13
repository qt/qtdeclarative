// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLWATCHER_H
#define QQMLWATCHER_H

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
#include <QtCore/qlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qhash.h>
#include <QtCore/qset.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQmlWatchProxy;
class QQmlExpression;
class QQmlContext;
class QMetaProperty;

class QQmlWatcher : public QObject
{
    Q_OBJECT
public:
    QQmlWatcher(QObject * = nullptr);

    bool addWatch(int id, quint32 objectId);
    bool addWatch(int id, quint32 objectId, const QByteArray &property);
    bool addWatch(int id, quint32 objectId, const QString &expr);

    bool removeWatch(int id);

Q_SIGNALS:
    void propertyChanged(int id, int objectId, const QMetaProperty &property, const QVariant &value);

private:
    friend class QQmlWatchProxy;
    void addPropertyWatch(int id, QObject *object, quint32 objectId, const QMetaProperty &property);

    QHash<int, QList<QPointer<QQmlWatchProxy> > > m_proxies;
};

QT_END_NAMESPACE

#endif // QQMLWATCHER_H
