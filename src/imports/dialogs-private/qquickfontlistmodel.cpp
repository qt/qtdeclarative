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

#include "qquickfontlistmodel_p.h"
#include <QtGui/qfontdatabase.h>
#include <QtQml/qqmlcontext.h>
#include <private/qqmlengine_p.h>
#include <private/qv8engine_p.h>
#include <private/qv4value_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4object_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

class QQuickFontListModelPrivate
{
    Q_DECLARE_PUBLIC(QQuickFontListModel)

public:
    QQuickFontListModelPrivate(QQuickFontListModel *q)
        : q_ptr(q), ws(QFontDatabase::Any)
        , options(QSharedPointer<QFontDialogOptions>(new QFontDialogOptions()))
    {}

    QQuickFontListModel *q_ptr;
    QFontDatabase db;
    QFontDatabase::WritingSystem ws;
    QSharedPointer<QFontDialogOptions> options;
    QStringList families;
    QHash<int, QByteArray> roleNames;
    ~QQuickFontListModelPrivate() {}
    void init();
};


void QQuickFontListModelPrivate::init()
{
    Q_Q(QQuickFontListModel);

    families = db.families();

    emit q->rowCountChanged();
    emit q->writingSystemChanged();
}

QQuickFontListModel::QQuickFontListModel(QObject *parent)
    : QAbstractListModel(parent), d_ptr(new QQuickFontListModelPrivate(this))
{
    Q_D(QQuickFontListModel);
    d->roleNames[FontFamilyRole] = "family";
    d->init();
}

QQuickFontListModel::~QQuickFontListModel()
{
}

QVariant QQuickFontListModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QQuickFontListModel);
    QVariant rv;

    if (index.row() >= d->families.size())
        return rv;

    switch (role)
    {
        case FontFamilyRole:
            rv = d->families.at(index.row());
            break;
        default:
            break;
    }
    return rv;
}

QHash<int, QByteArray> QQuickFontListModel::roleNames() const
{
    Q_D(const QQuickFontListModel);
    return d->roleNames;
}

int QQuickFontListModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QQuickFontListModel);
    Q_UNUSED(parent);
    return d->families.size();
}

QModelIndex QQuickFontListModel::index(int row, int , const QModelIndex &) const
{
    return createIndex(row, 0);
}

QString QQuickFontListModel::writingSystem() const
{
    Q_D(const QQuickFontListModel);
    return QFontDatabase::writingSystemName(d->ws);
}

void QQuickFontListModel::setWritingSystem(const QString &wSystem)
{
    Q_D(QQuickFontListModel);

    if (wSystem == writingSystem())
        return;

    QList<QFontDatabase::WritingSystem> wss;
    wss << QFontDatabase::Any;
    wss << d->db.writingSystems();
    QFontDatabase::WritingSystem ws;
    foreach (ws, wss) {
        if (wSystem == QFontDatabase::writingSystemName(ws)) {
            d->ws = ws;
            updateFamilies();
            return;
        }
    }
}

void QQuickFontListModel::updateFamilies()
{
    Q_D(QQuickFontListModel);

    beginResetModel();
    const QFontDialogOptions::FontDialogOptions scalableMask = (QFontDialogOptions::FontDialogOptions)(QFontDialogOptions::ScalableFonts | QFontDialogOptions::NonScalableFonts);
    const QFontDialogOptions::FontDialogOptions spacingMask = (QFontDialogOptions::FontDialogOptions)(QFontDialogOptions::ProportionalFonts | QFontDialogOptions::MonospacedFonts);
    const QFontDialogOptions::FontDialogOptions options = d->options->options();

    d->families.clear();
    foreach (const QString &family, d->db.families(d->ws)) {
        if ((options & scalableMask) && (options & scalableMask) != scalableMask) {
            if (bool(options & QFontDialogOptions::ScalableFonts) != d->db.isSmoothlyScalable(family))
                continue;
        }
        if ((options & spacingMask) && (options & spacingMask) != spacingMask) {
            if (bool(options & QFontDialogOptions::MonospacedFonts) != d->db.isFixedPitch(family))
                continue;
        }
        d->families << family;
    }
    endResetModel();
}

bool QQuickFontListModel::scalableFonts() const
{
    Q_D(const QQuickFontListModel);
    return d->options->testOption(QFontDialogOptions::ScalableFonts);
}

bool QQuickFontListModel::nonScalableFonts() const
{
    Q_D(const QQuickFontListModel);
    return d->options->testOption(QFontDialogOptions::NonScalableFonts);
}

bool QQuickFontListModel::monospacedFonts() const
{
    Q_D(const QQuickFontListModel);
    return d->options->testOption(QFontDialogOptions::MonospacedFonts);
}

bool QQuickFontListModel::proportionalFonts() const
{
    Q_D(const QQuickFontListModel);
    return d->options->testOption(QFontDialogOptions::ProportionalFonts);
}

QQmlV4Handle QQuickFontListModel::get(int idx) const
{
    Q_D(const QQuickFontListModel);

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

QQmlV4Handle QQuickFontListModel::pointSizes()
{
    QQmlEngine *engine = qmlContext(this)->engine();
    QV8Engine *v8engine = QQmlEnginePrivate::getV8Engine(engine);
    ExecutionEngine *v4engine = QV8Engine::getV4(v8engine);
    Scope scope(v4engine);

    QList<int> pss = QFontDatabase::standardSizes();
    int size = pss.size();

    Scoped<QV4::ArrayObject> a(scope, v4engine->newArrayObject());
    a->arrayReserve(size);
    a->arrayDataLen = size;
    for (int i = 0; i < size; ++i)
        a->arrayData[i].value = Primitive::fromInt32(pss.at(i));
    a->setArrayLengthUnchecked(size);

    return QQmlV4Handle(ScopedValue(scope, a.asReturnedValue()));
}

void QQuickFontListModel::classBegin()
{
}

void QQuickFontListModel::componentComplete()
{
}

void QQuickFontListModel::setScalableFonts(bool arg)
{
    Q_D(QQuickFontListModel);
    d->options->setOption(QFontDialogOptions::ScalableFonts, arg);
    updateFamilies();
    emit scalableFontsChanged();
}

void QQuickFontListModel::setNonScalableFonts(bool arg)
{
    Q_D(QQuickFontListModel);
    d->options->setOption(QFontDialogOptions::NonScalableFonts, arg);
    updateFamilies();
    emit nonScalableFontsChanged();
}

void QQuickFontListModel::setMonospacedFonts(bool arg)
{
    Q_D(QQuickFontListModel);
    d->options->setOption(QFontDialogOptions::MonospacedFonts, arg);
    updateFamilies();
    emit monospacedFontsChanged();
}

void QQuickFontListModel::setProportionalFonts(bool arg)
{
    Q_D(QQuickFontListModel);
    d->options->setOption(QFontDialogOptions::ProportionalFonts, arg);
    updateFamilies();
    emit proportionalFontsChanged();
}

QT_END_NAMESPACE
