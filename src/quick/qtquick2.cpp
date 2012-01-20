/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtquick2_p.h"
#include <private/qdeclarativeengine_p.h>
#include <private/qdeclarativeutilmodule_p.h>
#include <private/qdeclarativevaluetype_p.h>
#include <private/qquickitemsmodule_p.h>
#include <private/qquickparticlesmodule_p.h>
#include <private/qquickwindowmodule_p.h>

#include <private/qdeclarativeenginedebugservice_p.h>
#include <private/qdeclarativedebugstatesdelegate_p.h>
#include <private/qdeclarativebinding_p.h>
#include <private/qdeclarativecontext_p.h>
#include <QtQuick/private/qdeclarativepropertychanges_p.h>
#include <QtQuick/private/qdeclarativestate_p.h>
#include <qdeclarativeproperty.h>
#include <QtCore/QWeakPointer>

QT_BEGIN_NAMESPACE

class QDeclarativeQtQuick2DebugStatesDelegate : public QDeclarativeDebugStatesDelegate
{
public:
    QDeclarativeQtQuick2DebugStatesDelegate();
    virtual ~QDeclarativeQtQuick2DebugStatesDelegate();
    virtual void buildStatesList(QDeclarativeContext *ctxt, bool cleanList);
    virtual void updateBinding(QDeclarativeContext *context,
                               const QDeclarativeProperty &property,
                               const QVariant &expression, bool isLiteralValue,
                               const QString &fileName, int line, int column,
                               bool *isBaseState);
    virtual bool setBindingForInvalidProperty(QObject *object,
                                              const QString &propertyName,
                                              const QVariant &expression,
                                              bool isLiteralValue);
    virtual void resetBindingForInvalidProperty(QObject *object,
                                                const QString &propertyName);

private:
    void buildStatesList(QObject *obj);

    QList<QWeakPointer<QDeclarativeState> > m_allStates;
};

QDeclarativeQtQuick2DebugStatesDelegate::QDeclarativeQtQuick2DebugStatesDelegate()
{
}

QDeclarativeQtQuick2DebugStatesDelegate::~QDeclarativeQtQuick2DebugStatesDelegate()
{
}

void QDeclarativeQtQuick2DebugStatesDelegate::buildStatesList(QDeclarativeContext *ctxt, bool cleanList)
{
    if (cleanList)
        m_allStates.clear();

    QDeclarativeContextPrivate *ctxtPriv = QDeclarativeContextPrivate::get(ctxt);
    for (int ii = 0; ii < ctxtPriv->instances.count(); ++ii) {
        buildStatesList(ctxtPriv->instances.at(ii));
    }

    QDeclarativeContextData *child = QDeclarativeContextData::get(ctxt)->childContexts;
    while (child) {
        buildStatesList(child->asQDeclarativeContext());
        child = child->nextChild;
    }
}

void QDeclarativeQtQuick2DebugStatesDelegate::buildStatesList(QObject *obj)
{
    if (QDeclarativeState *state = qobject_cast<QDeclarativeState *>(obj)) {
        m_allStates.append(state);
    }

    QObjectList children = obj->children();
    for (int ii = 0; ii < children.count(); ++ii) {
        buildStatesList(children.at(ii));
    }
}

void QDeclarativeQtQuick2DebugStatesDelegate::updateBinding(QDeclarativeContext *context,
                                                            const QDeclarativeProperty &property,
                                                            const QVariant &expression, bool isLiteralValue,
                                                            const QString &fileName, int line, int column,
                                                            bool *inBaseState)
{
    QObject *object = property.object();
    QString propertyName = property.name();
    foreach (QWeakPointer<QDeclarativeState> statePointer, m_allStates) {
        if (QDeclarativeState *state = statePointer.data()) {
            // here we assume that the revert list on itself defines the base state
            if (state->isStateActive() && state->containsPropertyInRevertList(object, propertyName)) {
                *inBaseState = false;

                QDeclarativeBinding *newBinding = 0;
                if (!isLiteralValue) {
                    newBinding = new QDeclarativeBinding(expression.toString(), object, context);
                    newBinding->setTarget(property);
                    newBinding->setNotifyOnValueChanged(true);
                    newBinding->setSourceLocation(fileName, line, column);
                }

                state->changeBindingInRevertList(object, propertyName, newBinding);

                if (isLiteralValue)
                    state->changeValueInRevertList(object, propertyName, expression);
            }
        }
    }
}

bool QDeclarativeQtQuick2DebugStatesDelegate::setBindingForInvalidProperty(QObject *object,
                                                                           const QString &propertyName,
                                                                           const QVariant &expression,
                                                                           bool isLiteralValue)
{
    if (QDeclarativePropertyChanges *propertyChanges = qobject_cast<QDeclarativePropertyChanges *>(object)) {
        if (isLiteralValue)
            propertyChanges->changeValue(propertyName, expression);
        else
            propertyChanges->changeExpression(propertyName, expression.toString());
        return true;
    } else {
        return false;
    }
}

void QDeclarativeQtQuick2DebugStatesDelegate::resetBindingForInvalidProperty(QObject *object, const QString &propertyName)
{
    if (QDeclarativePropertyChanges *propertyChanges = qobject_cast<QDeclarativePropertyChanges *>(object)) {
        propertyChanges->removeProperty(propertyName);
    }
}


void QDeclarativeQtQuick2Module::defineModule()
{
    QDeclarativeUtilModule::defineModule();
    QDeclarativeEnginePrivate::defineModule();
    QQuickItemsModule::defineModule();
    QQuickParticlesModule::defineModule();
    QQuickWindowModule::defineModule();
    QDeclarativeValueTypeFactory::registerValueTypes();

    if (QDeclarativeEngineDebugService::isDebuggingEnabled()) {
        QDeclarativeEngineDebugService::instance()->setStatesDelegate(
                    new QDeclarativeQtQuick2DebugStatesDelegate);
    }
}

QT_END_NAMESPACE

