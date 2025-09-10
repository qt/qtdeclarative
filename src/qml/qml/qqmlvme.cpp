// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlvme_p.h"

#include <private/qmetaobjectbuilder_p.h>
#include "qqmlengine.h"
#include <private/qfinitestack_p.h>
#include <QtQml/private/qqmlcomponent_p.h>

#include <QStack>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <QtCore/qdebug.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qvarlengtharray.h>
#include <QtQml/qjsvalue.h>

QT_BEGIN_NAMESPACE

bool QQmlVME::s_enableComponentComplete = true;

void QQmlVME::enableComponentComplete()
{
    s_enableComponentComplete = true;
}

void QQmlVME::disableComponentComplete()
{
    s_enableComponentComplete = false;
}

bool QQmlVME::componentCompleteEnabled()
{
    return s_enableComponentComplete;
}

QQmlVMEGuard::QQmlVMEGuard()
: m_objectCount(0), m_objects(nullptr), m_contextCount(0), m_contexts(nullptr)
{
}

QQmlVMEGuard::~QQmlVMEGuard()
{
    clear();
}

void QQmlVMEGuard::guard(QQmlObjectCreator *creator)
{
    clear();

    QFiniteStack<QQmlGuard<QObject> > &objects = creator->allCreatedObjects();
    m_objectCount = objects.count();
    m_objects = new QQmlGuard<QObject>[m_objectCount];
    for (int ii = 0; ii < m_objectCount; ++ii)
        m_objects[ii] = objects[ii];

    m_contextCount = 1;
    m_contexts = new QQmlGuardedContextData[m_contextCount];
    m_contexts[0] = creator->parentContextData();
}

void QQmlVMEGuard::clear()
{
    delete [] m_objects;
    delete [] m_contexts;

    m_objectCount = 0;
    m_objects = nullptr;
    m_contextCount = 0;
    m_contexts = nullptr;
}

bool QQmlVMEGuard::isOK() const
{
    for (int ii = 0; ii < m_objectCount; ++ii)
        if (m_objects[ii].isNull())
            return false;

    for (int ii = 0; ii < m_contextCount; ++ii)
        if (m_contexts[ii].isNull() || !m_contexts[ii]->engine())
            return false;

    return true;
}

QT_END_NAMESPACE
