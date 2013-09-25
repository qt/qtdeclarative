/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick.Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickwritingsystemlistmodel_p.h"
#include <QtGui/qfontdatabase.h>
#include <QtQml/qqmlcontext.h>
#include <private/qqmlengine_p.h>
#include <private/qv8engine_p.h>
#include <private/qv4value_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4object_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

class QQuickWritingSystemListModelPrivate
{
    Q_DECLARE_PUBLIC(QQuickWritingSystemListModel)

public:
    QQuickWritingSystemListModelPrivate(QQuickWritingSystemListModel *q)
        : q_ptr(q)
    {}

    QQuickWritingSystemListModel *q_ptr;
    QList<QFontDatabase::WritingSystem> wss;
    QHash<int, QByteArray> roleNames;
    ~QQuickWritingSystemListModelPrivate() {}
    void init();
};


void QQuickWritingSystemListModelPrivate::init()
{
    Q_Q(QQuickWritingSystemListModel);
    wss << QFontDatabase::Any;
    QFontDatabase db;
    wss << db.writingSystems();

    emit q->rowCountChanged();
    emit q->writingSystemsChanged();
}

QQuickWritingSystemListModel::QQuickWritingSystemListModel(QObject *parent)
    : QAbstractListModel(parent), d_ptr(new QQuickWritingSystemListModelPrivate(this))
{
    Q_D(QQuickWritingSystemListModel);
    d->roleNames[WritingSystemNameRole] = "name";
    d->roleNames[WritingSystemSampleRole] = "sample";
    d->init();
}

QQuickWritingSystemListModel::~QQuickWritingSystemListModel()
{
}

QVariant QQuickWritingSystemListModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QQuickWritingSystemListModel);
    QVariant rv;

    if (index.row() >= d->wss.size())
        return rv;

    switch (role)
    {
        case WritingSystemNameRole:
            rv = QFontDatabase::writingSystemName(d->wss.at(index.row()));
            break;
        case WritingSystemSampleRole:
            rv = QFontDatabase::writingSystemSample(d->wss.at(index.row()));
            break;
        default:
            break;
    }
    return rv;
}

QHash<int, QByteArray> QQuickWritingSystemListModel::roleNames() const
{
    Q_D(const QQuickWritingSystemListModel);
    return d->roleNames;
}

int QQuickWritingSystemListModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QQuickWritingSystemListModel);
    Q_UNUSED(parent);
    return d->wss.size();
}

QModelIndex QQuickWritingSystemListModel::index(int row, int , const QModelIndex &) const
{
    return createIndex(row, 0);
}

QStringList QQuickWritingSystemListModel::writingSystems() const
{
    Q_D(const QQuickWritingSystemListModel);
    QStringList result;
    QFontDatabase::WritingSystem ws;
    foreach (ws, d->wss)
        result.append(QFontDatabase::writingSystemName(ws));

    return result;
}

QQmlV4Handle QQuickWritingSystemListModel::get(int idx) const
{
    Q_D(const QQuickWritingSystemListModel);

    if (idx < 0 || idx >= count())
        return QQmlV4Handle(Encode::undefined());

    QQmlEngine *engine = qmlContext(this)->engine();
    QV8Engine *v8engine = QQmlEnginePrivate::getV8Engine(engine);
    ExecutionEngine *v4engine = QV8Engine::getV4(v8engine);
    Scope scope(v4engine);
    ScopedObject o(scope, v4engine->newObject());
    ScopedString s(scope);
    for (int ii = 0; ii < d->roleNames.keys().count(); ++ii) {
        Property *p = o->insertMember((s = v4engine->newIdentifier(d->roleNames[Qt::UserRole + ii + 1])), PropertyAttributes());
        p->value = v8engine->fromVariant(data(index(idx, 0), Qt::UserRole + ii + 1));
    }

    return QQmlV4Handle(o);
}

void QQuickWritingSystemListModel::classBegin()
{
}

void QQuickWritingSystemListModel::componentComplete()
{
}

QT_END_NAMESPACE
