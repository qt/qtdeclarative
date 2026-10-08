// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qquickpackage_p.h"

#include <private/qobject_p.h>
#include <private/qqmlguard_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Package
    \nativetype QQuickPackage
    \inqmlmodule QtQml.Models
    \ingroup qtquick-models
    \brief Specifies a collection of named items.

    The Package type is used in conjunction with
    DelegateModel to enable delegates with a shared context
    to be provided to multiple views.

    Any item within a Package may be assigned a name via the
    \l{Package::name}{Package.name} attached property.

    The example below creates a Package containing two named items;
    \e list and \e grid.  The third item in the package (the \l Rectangle) is parented to whichever
    delegate it should appear in.  This allows an item to move
    between views.

    \snippet package/Delegate.qml 0

    These named items are used as the delegates by the two views who
    reference the special \l{DelegateModel::parts} property to select
    a model which provides the chosen delegate.

    \snippet package/view.qml 0

    \note Package is part of QtQml.Models since version 2.14 and part of QtQuick since version 2.0.
    Importing Package via QtQuick is deprecated since Qt 5.14.

    \sa {Qt Quick Examples - Views}, {Qt Qml}
*/

/*!
    \qmlattachedproperty string QtQml.Models::Package::name
    This attached property holds the name of an item within a Package.
*/


class QQuickPackagePrivate : public QObjectPrivate
{
public:
    QQuickPackagePrivate() {}

    struct DataGuard : public QQmlGuard<QObject>
    {
        DataGuard(QObject *obj, QList<DataGuard> *l) : QQmlGuard<QObject>(DataGuard::objectDestroyedImpl, nullptr), list(l) { (QQmlGuard<QObject>&)*this = obj; }
        QList<DataGuard> *list;

    private:
        static void objectDestroyedImpl(QQmlGuardImpl *guard) {
            auto This = static_cast<DataGuard *>(guard);
            // we assume priv will always be destroyed after objectDestroyed calls
            This->list->removeOne(*This);
        }
    };

    QList<DataGuard> dataList;
    static void data_append(QQmlListProperty<QObject> *prop, QObject *o) {
        QList<DataGuard> *list = static_cast<QList<DataGuard> *>(prop->data);
        list->append(DataGuard(o, list));
    }
    static void data_clear(QQmlListProperty<QObject> *prop) {
        QList<DataGuard> *list = static_cast<QList<DataGuard> *>(prop->data);
        list->clear();
    }
    static QObject *data_at(QQmlListProperty<QObject> *prop, qsizetype index) {
        QList<DataGuard> *list = static_cast<QList<DataGuard> *>(prop->data);
        return list->at(index);
    }
    static qsizetype data_count(QQmlListProperty<QObject> *prop) {
        QList<DataGuard> *list = static_cast<QList<DataGuard> *>(prop->data);
        return list->size();
    }
    static void data_replace(QQmlListProperty<QObject> *prop, qsizetype index, QObject *o) {
        QList<DataGuard> *list = static_cast<QList<DataGuard> *>(prop->data);
        list->replace(index, DataGuard(o, list));
    }
    static void data_removeLast(QQmlListProperty<QObject> *prop) {
        QList<DataGuard> *list = static_cast<QList<DataGuard> *>(prop->data);
        list->removeLast();
    }
};

QHash<QObject *, QQuickPackageAttached *> QQuickPackageAttached::attached;

QQuickPackageAttached::QQuickPackageAttached(QObject *parent)
: QObject(parent)
{
    attached.insert(parent, this);
}

QQuickPackageAttached::~QQuickPackageAttached()
{
    attached.remove(parent());
}

QString QQuickPackageAttached::name() const
{
    return _name;
}

void QQuickPackageAttached::setName(const QString &n)
{
    _name = n;
}

QQuickPackage::QQuickPackage(QObject *parent)
    : QObject(*(new QQuickPackagePrivate), parent)
{
}

QQmlListProperty<QObject> QQuickPackage::data()
{
    Q_D(QQuickPackage);
    return QQmlListProperty<QObject>(this, &d->dataList,
                                     QQuickPackagePrivate::data_append,
                                     QQuickPackagePrivate::data_count,
                                     QQuickPackagePrivate::data_at,
                                     QQuickPackagePrivate::data_clear,
                                     QQuickPackagePrivate::data_replace,
                                     QQuickPackagePrivate::data_removeLast);
}

bool QQuickPackage::hasPart(const QString &name)
{
    Q_D(QQuickPackage);
    for (int ii = 0; ii < d->dataList.size(); ++ii) {
        QObject *obj = d->dataList.at(ii);
        QQuickPackageAttached *a = QQuickPackageAttached::attached.value(obj);
        if (a && a->name() == name)
            return true;
    }
    return false;
}

QObject *QQuickPackage::part(const QString &name)
{
    Q_D(QQuickPackage);
    if (name.isEmpty() && !d->dataList.isEmpty())
        return d->dataList.at(0);

    for (int ii = 0; ii < d->dataList.size(); ++ii) {
        QObject *obj = d->dataList.at(ii);
        QQuickPackageAttached *a = QQuickPackageAttached::attached.value(obj);
        if (a && a->name() == name)
            return obj;
    }

    if (name == QLatin1String("default") && !d->dataList.isEmpty())
        return d->dataList.at(0);

    return nullptr;
}

QQuickPackageAttached *QQuickPackage::qmlAttachedProperties(QObject *o)
{
    return new QQuickPackageAttached(o);
}



QT_END_NAMESPACE

#include "moc_qquickpackage_p.cpp"
