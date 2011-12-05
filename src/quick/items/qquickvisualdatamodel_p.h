// Commit: ac5c099cc3c5b8c7eec7a49fdeb8a21037230350
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

#ifndef QQUICKVISUALDATAMODEL_P_H
#define QQUICKVISUALDATAMODEL_P_H

#include <private/qdeclarativelistcompositor_p.h>
#include <private/qquickvisualitemmodel_p.h>


#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qstringlist.h>

#include <private/qv8engine_p.h>

QT_BEGIN_HEADER

Q_DECLARE_METATYPE(QModelIndex)

QT_BEGIN_NAMESPACE

class QDeclarativeChangeSet;
class QDeclarativeComponent;
class QDeclarativePackage;
class QDeclarativeV8Function;
class QQuickVisualDataGroup;
class QQuickVisualDataModelAttached;
class QQuickVisualDataModelPrivate;


class Q_QUICK_EXPORT QQuickVisualDataModel : public QQuickVisualModel, public QDeclarativeParserStatus
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickVisualDataModel)

    Q_PROPERTY(QVariant model READ model WRITE setModel)
    Q_PROPERTY(QDeclarativeComponent *delegate READ delegate WRITE setDelegate)
    Q_PROPERTY(QString filterOnGroup READ filterGroup WRITE setFilterGroup NOTIFY filterGroupChanged RESET resetFilterGroup)
    Q_PROPERTY(QQuickVisualDataGroup *items READ items CONSTANT)
    Q_PROPERTY(QQuickVisualDataGroup *persistedItems READ persistedItems CONSTANT)
    Q_PROPERTY(QDeclarativeListProperty<QQuickVisualDataGroup> groups READ groups CONSTANT)
    Q_PROPERTY(QObject *parts READ parts CONSTANT)
    Q_PROPERTY(QVariant rootIndex READ rootIndex WRITE setRootIndex NOTIFY rootIndexChanged)
    Q_CLASSINFO("DefaultProperty", "delegate")
    Q_INTERFACES(QDeclarativeParserStatus)
public:
    QQuickVisualDataModel();
    QQuickVisualDataModel(QDeclarativeContext *, QObject *parent=0);
    virtual ~QQuickVisualDataModel();

    void classBegin();
    void componentComplete();

    QVariant model() const;
    void setModel(const QVariant &);

    QDeclarativeComponent *delegate() const;
    void setDelegate(QDeclarativeComponent *);

    QVariant rootIndex() const;
    void setRootIndex(const QVariant &root);

    Q_INVOKABLE QVariant modelIndex(int idx) const;
    Q_INVOKABLE QVariant parentModelIndex() const;

    int count() const;
    bool isValid() const { return delegate() != 0; }
    QQuickItem *item(int index, bool asynchronous=false);
    ReleaseFlags release(QQuickItem *item);
    virtual QString stringValue(int index, const QString &role);
    virtual void setWatchedRoles(QList<QByteArray> roles);

    int indexOf(QQuickItem *item, QObject *objectContext) const;

    QString filterGroup() const;
    void setFilterGroup(const QString &group);
    void resetFilterGroup();

    QQuickVisualDataGroup *items();
    QQuickVisualDataGroup *persistedItems();
    QDeclarativeListProperty<QQuickVisualDataGroup> groups();
    QObject *parts();

    bool event(QEvent *);

    static QQuickVisualDataModelAttached *qmlAttachedProperties(QObject *obj);

Q_SIGNALS:
    void filterGroupChanged();
    void defaultGroupsChanged();
    void rootIndexChanged();

private Q_SLOTS:
    void _q_itemsChanged(int index, int count);
    void _q_itemsInserted(int index, int count);
    void _q_itemsRemoved(int index, int count);
    void _q_itemsMoved(int from, int to, int count);
    void _q_modelReset(int oldCount, int newCount);
private:
    Q_DISABLE_COPY(QQuickVisualDataModel)
};

class QQuickVisualDataGroupPrivate;
class Q_AUTOTEST_EXPORT QQuickVisualDataGroup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool includeByDefault READ defaultInclude WRITE setDefaultInclude NOTIFY defaultIncludeChanged)
public:
    QQuickVisualDataGroup(QObject *parent = 0);
    QQuickVisualDataGroup(const QString &name, QQuickVisualDataModel *model, int compositorType, QObject *parent = 0);
    ~QQuickVisualDataGroup();

    QString name() const;
    void setName(const QString &name);

    int count() const;

    bool defaultInclude() const;
    void setDefaultInclude(bool include);

    Q_INVOKABLE QDeclarativeV8Handle get(int index);
    Q_INVOKABLE QObject *create(int index);

public Q_SLOTS:
    void remove(QDeclarativeV8Function *);
    void addGroups(QDeclarativeV8Function *);
    void removeGroups(QDeclarativeV8Function *);
    void setGroups(QDeclarativeV8Function *);
    void move(QDeclarativeV8Function *);

Q_SIGNALS:
    void countChanged();
    void nameChanged();
    void defaultIncludeChanged();
    void changed(const QDeclarativeV8Handle &removed, const QDeclarativeV8Handle &inserted);
private:
    Q_DECLARE_PRIVATE(QQuickVisualDataGroup)
};

class QQuickVisualDataModelCacheItem;
class QQuickVisualDataModelAttachedMetaObject;
class QQuickVisualDataModelAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickVisualDataModel *model READ model NOTIFY modelChanged)
    Q_PROPERTY(QStringList groups READ groups WRITE setGroups NOTIFY groupsChanged)
public:
    QQuickVisualDataModelAttached(QObject *parent)
        : QObject(parent)
        , m_cacheItem(0)
        , m_previousGroups(0)
        , m_modelChanged(false)
    {}
    ~QQuickVisualDataModelAttached() { attachedProperties.remove(parent()); }

    void setCacheItem(QQuickVisualDataModelCacheItem *item);

    QQuickVisualDataModel *model() const;

    QStringList groups() const;
    void setGroups(const QStringList &groups);

    void emitChanges();

    static QQuickVisualDataModelAttached *properties(QObject *obj)
    {
        QQuickVisualDataModelAttached *rv = attachedProperties.value(obj);
        if (!rv) {
            rv = new QQuickVisualDataModelAttached(obj);
            attachedProperties.insert(obj, rv);
        }
        return rv;
    }

Q_SIGNALS:
    void modelChanged();
    void groupsChanged();

public:
    QQuickVisualDataModelCacheItem *m_cacheItem;
    int m_previousGroups;
    int m_previousIndex[QDeclarativeListCompositor::MaximumGroupCount];
    bool m_modelChanged;

    static QHash<QObject*, QQuickVisualDataModelAttached*> attachedProperties;

    friend class QQuickVisualDataModelAttachedMetaObject;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickVisualDataModel)
QML_DECLARE_TYPEINFO(QQuickVisualDataModel, QML_HAS_ATTACHED_PROPERTIES)
QML_DECLARE_TYPE(QQuickVisualDataGroup)

QT_END_HEADER

#endif // QQUICKVISUALDATAMODEL_P_H
