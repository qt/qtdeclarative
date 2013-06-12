/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef APIPREAMBLE_P_H
#define APIPREAMBLE_P_H

#include <private/qv8_p.h>
#include "qv8engine_p.h"

QT_BEGIN_NAMESPACE

/**
  \internal
  Class used to switch to the right isolate. It does the same thing as v8::Isolate::Scope but
  it checks for a null engine.
  \attention We decided to put context switching "up" which means that it should be as high
  as possible on call stack. And it should be switched at most once per public API function call.
*/
class QScriptIsolate {
public:
    // OperationMode was introduced to reduce number of checking for a null engine pointer. If we
    // know that given pointer is not null than we should pass NotNullEngine as constructor argument
    // that would nicely remove checking on compilation time.
    enum OperationMode {Default, NotNullEngine};
    inline QScriptIsolate(const QV8Engine *engine, const OperationMode mode = Default)
        : m_engine(engine)
        , m_mode(mode)
    {
        if (m_mode == NotNullEngine || m_engine) {
            Q_ASSERT(m_engine);
            m_engine->context()->Enter();
        }
    }

    inline ~QScriptIsolate()
    {
        if (m_mode == NotNullEngine || m_engine) {
            m_engine->context()->Exit();
        }
    }

private:
    Q_DISABLE_COPY(QScriptIsolate)
    const QV8Engine *m_engine;
    const OperationMode m_mode;
};


QT_END_NAMESPACE

#endif // APIPREAMBLE_P_H
