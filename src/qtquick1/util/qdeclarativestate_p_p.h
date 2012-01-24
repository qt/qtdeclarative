/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVESTATE_P_H
#define QDECLARATIVESTATE_P_H

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

#include "QtQuick1/private/qdeclarativestate_p.h"

#include "QtQuick1/private/qdeclarativeanimation_p_p.h"
#include "QtQuick1/private/qdeclarativetransitionmanager_p_p.h"

#include <QtDeclarative/private/qdeclarativeproperty_p.h>
#include <QtDeclarative/private/qdeclarativeguard_p.h>

#include <QtDeclarative/private/qdeclarativebinding_p.h>

#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QDeclarative1SimpleAction
{
public:
    enum State { StartState, EndState };
    QDeclarative1SimpleAction(const QDeclarative1Action &a, State state = StartState)
    {
        m_property = a.property;
        m_specifiedObject = a.specifiedObject;
        m_specifiedProperty = a.specifiedProperty;
        m_event = a.event;
        if (state == StartState) {
            m_value = a.fromValue;
            if (QDeclarativePropertyPrivate::binding(m_property)) {
                m_binding = QDeclarativeAbstractBinding::getPointer(QDeclarativePropertyPrivate::binding(m_property));
            }
            m_reverseEvent = true;
        } else {
            m_value = a.toValue;
            m_binding = a.toBinding;
            m_reverseEvent = false;
        }
    }

    ~QDeclarative1SimpleAction()
    {
    }

    QDeclarative1SimpleAction(const QDeclarative1SimpleAction &other)
        :  m_property(other.m_property),
        m_value(other.m_value),
        m_binding(QDeclarativeAbstractBinding::getPointer(other.binding())),
        m_specifiedObject(other.m_specifiedObject),
        m_specifiedProperty(other.m_specifiedProperty),
        m_event(other.m_event),
        m_reverseEvent(other.m_reverseEvent)
    {
    }

    QDeclarative1SimpleAction &operator =(const QDeclarative1SimpleAction &other)
    {
        m_property = other.m_property;
        m_value = other.m_value;
        m_binding = QDeclarativeAbstractBinding::getPointer(other.binding());
        m_specifiedObject = other.m_specifiedObject;
        m_specifiedProperty = other.m_specifiedProperty;
        m_event = other.m_event;
        m_reverseEvent = other.m_reverseEvent;

        return *this;
    }

    void setProperty(const QDeclarativeProperty &property)
    {
        m_property = property;
    }

    const QDeclarativeProperty &property() const
    {
        return m_property;
    }

    void setValue(const QVariant &value)
    {
        m_value = value;
    }

    const QVariant &value() const
    {
        return m_value;
    }

    void setBinding(QDeclarativeAbstractBinding *binding)
    {
        m_binding = QDeclarativeAbstractBinding::getPointer(binding);
    }

    QDeclarativeAbstractBinding *binding() const
    {
        return m_binding.data();
    }

    QObject *specifiedObject() const
    {
        return m_specifiedObject;
    }

    const QString &specifiedProperty() const
    {
        return m_specifiedProperty;
    }

    QDeclarative1ActionEvent *event() const
    {
        return m_event;
    }

    bool reverseEvent() const
    {
        return m_reverseEvent;
    }

private:
    QDeclarativeProperty m_property;
    QVariant m_value;
    QDeclarativeAbstractBinding::Pointer m_binding;
    QObject *m_specifiedObject;
    QString m_specifiedProperty;
    QDeclarative1ActionEvent *m_event;
    bool m_reverseEvent;
};

class QDeclarative1StateOperationPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1StateOperation)

public:

    QDeclarative1StateOperationPrivate()
    : m_state(0) {}

    QDeclarative1State *m_state;
};

class QDeclarative1StatePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1State)

public:
    QDeclarative1StatePrivate()
    : when(0), named(false), inState(false), group(0) {}

    typedef QList<QDeclarative1SimpleAction> SimpleActionList;

    QString name;
    QDeclarativeBinding *when;
    bool named;

    struct OperationGuard : public QDeclarativeGuard<QDeclarative1StateOperation>
    {
        OperationGuard(QObject *obj, QList<OperationGuard> *l) : list(l) { (QDeclarativeGuard<QObject>&)*this = obj; }
        QList<OperationGuard> *list;
        void objectDestroyed(QDeclarative1StateOperation *) {
            // we assume priv will always be destroyed after objectDestroyed calls
            list->removeOne(*this);
        }
    };
    QList<OperationGuard> operations;

    static void operations_append(QDeclarativeListProperty<QDeclarative1StateOperation> *prop, QDeclarative1StateOperation *op) {
        QList<OperationGuard> *list = static_cast<QList<OperationGuard> *>(prop->data);
        op->setState(qobject_cast<QDeclarative1State*>(prop->object));
        list->append(OperationGuard(op, list));
    }
    static void operations_clear(QDeclarativeListProperty<QDeclarative1StateOperation> *prop) {
        QList<OperationGuard> *list = static_cast<QList<OperationGuard> *>(prop->data);
        QMutableListIterator<OperationGuard> listIterator(*list);
        while(listIterator.hasNext())
            listIterator.next()->setState(0);
        list->clear();
    }
    static int operations_count(QDeclarativeListProperty<QDeclarative1StateOperation> *prop) {
        QList<OperationGuard> *list = static_cast<QList<OperationGuard> *>(prop->data);
        return list->count();
    }
    static QDeclarative1StateOperation *operations_at(QDeclarativeListProperty<QDeclarative1StateOperation> *prop, int index) {
        QList<OperationGuard> *list = static_cast<QList<OperationGuard> *>(prop->data);
        return list->at(index);
    }

    QDeclarative1TransitionManager transitionManager;

    SimpleActionList revertList;
    QList<QDeclarativeProperty> reverting;
    QString extends;
    mutable bool inState;
    QDeclarative1StateGroup *group;

    QDeclarative1StateOperation::ActionList generateActionList(QDeclarative1StateGroup *) const;
    void complete();
};

QT_END_NAMESPACE

#endif // QDECLARATIVESTATE_P_H
