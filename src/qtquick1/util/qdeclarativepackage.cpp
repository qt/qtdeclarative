/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "QtQuick1/private/qdeclarativepackage_p.h"

#include <private/qobject_p.h>
#include <QtDeclarative/private/qdeclarativeguard_p.h>

QT_BEGIN_NAMESPACE



/*!
    \qmlclass Package QDeclarative1Package
    \ingroup qml-working-with-data
    \brief Package provides a collection of named items.

    The Package class is used in conjunction with
    VisualDataModel to enable delegates with a shared context
    to be provided to multiple views.

    Any item within a Package may be assigned a name via the
    \l{Package::name}{Package.name} attached property.

    The example below creates a Package containing two named items;
    \e list and \e grid.  The third element in the package (the \l Rectangle) is parented to whichever
    delegate it should appear in.  This allows an item to move
    between views.

    \snippet examples/declarative/modelviews/package/Delegate.qml 0

    These named items are used as the delegates by the two views who
    reference the special \l{VisualDataModel::parts} property to select
    a model which provides the chosen delegate.

    \snippet examples/declarative/modelviews/package/view.qml 0

    \sa {declarative/modelviews/package}{Package example}, {demos/declarative/photoviewer}{Photo Viewer demo}, QtDeclarative
*/

/*!
    \qmlattachedproperty string Package::name
    This attached property holds the name of an item within a Package.
*/


class QDeclarative1PackagePrivate : public QObjectPrivate
{
public:
    QDeclarative1PackagePrivate() {}

    struct DataGuard : public QDeclarativeGuard<QObject>
    {
        DataGuard(QObject *obj, QList<DataGuard> *l) : list(l) { (QDeclarativeGuard<QObject>&)*this = obj; }
        QList<DataGuard> *list;
        void objectDestroyed(QObject *) {
            // we assume priv will always be destroyed after objectDestroyed calls
            list->removeOne(*this);
        }
    };

    QList<DataGuard> dataList;
    static void data_append(QDeclarativeListProperty<QObject> *prop, QObject *o) {
        QList<DataGuard> *list = static_cast<QList<DataGuard> *>(prop->data);
        list->append(DataGuard(o, list));
    }
    static void data_clear(QDeclarativeListProperty<QObject> *prop) {
        QList<DataGuard> *list = static_cast<QList<DataGuard> *>(prop->data);
        list->clear();
    }
    static QObject *data_at(QDeclarativeListProperty<QObject> *prop, int index) {
        QList<DataGuard> *list = static_cast<QList<DataGuard> *>(prop->data);
        return list->at(index);
    }
    static int data_count(QDeclarativeListProperty<QObject> *prop) {
        QList<DataGuard> *list = static_cast<QList<DataGuard> *>(prop->data);
        return list->count();
    }
};

QHash<QObject *, QDeclarative1PackageAttached *> QDeclarative1PackageAttached::attached;

QDeclarative1PackageAttached::QDeclarative1PackageAttached(QObject *parent)
: QObject(parent)
{
    attached.insert(parent, this);
}

QDeclarative1PackageAttached::~QDeclarative1PackageAttached()
{
    attached.remove(parent());
}

QString QDeclarative1PackageAttached::name() const 
{ 
    return _name; 
}

void QDeclarative1PackageAttached::setName(const QString &n) 
{ 
    _name = n; 
}

QDeclarative1Package::QDeclarative1Package(QObject *parent)
    : QObject(*(new QDeclarative1PackagePrivate), parent)
{
}

QDeclarative1Package::~QDeclarative1Package()
{
    Q_D(QDeclarative1Package);
    for (int ii = 0; ii < d->dataList.count(); ++ii) {
        QObject *obj = d->dataList.at(ii);
        obj->setParent(this);
    }
}

QDeclarativeListProperty<QObject> QDeclarative1Package::data()
{
    Q_D(QDeclarative1Package);
    return QDeclarativeListProperty<QObject>(this, &d->dataList, QDeclarative1PackagePrivate::data_append, 
                                                        QDeclarative1PackagePrivate::data_count, 
                                                        QDeclarative1PackagePrivate::data_at, 
                                                        QDeclarative1PackagePrivate::data_clear);
}

bool QDeclarative1Package::hasPart(const QString &name)
{
    Q_D(QDeclarative1Package);
    for (int ii = 0; ii < d->dataList.count(); ++ii) {
        QObject *obj = d->dataList.at(ii);
        QDeclarative1PackageAttached *a = QDeclarative1PackageAttached::attached.value(obj);
        if (a && a->name() == name)
            return true;
    }
    return false;
}

QObject *QDeclarative1Package::part(const QString &name)
{
    Q_D(QDeclarative1Package);
    if (name.isEmpty() && !d->dataList.isEmpty())
        return d->dataList.at(0);

    for (int ii = 0; ii < d->dataList.count(); ++ii) {
        QObject *obj = d->dataList.at(ii);
        QDeclarative1PackageAttached *a = QDeclarative1PackageAttached::attached.value(obj);
        if (a && a->name() == name)
            return obj;
    }

    if (name == QLatin1String("default") && !d->dataList.isEmpty())
        return d->dataList.at(0);

    return 0;
}

QDeclarative1PackageAttached *QDeclarative1Package::qmlAttachedProperties(QObject *o)
{
    return new QDeclarative1PackageAttached(o);
}





QT_END_NAMESPACE
