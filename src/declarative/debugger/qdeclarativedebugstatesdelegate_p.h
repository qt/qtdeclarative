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

#ifndef QDECLARATIVEDEBUGSTATESDELEGATE_P_H
#define QDECLARATIVEDEBUGSTATESDELEGATE_P_H

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

#include <private/qdeclarativeglobal_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativeContext;
class QDeclarativeProperty;
class QObject;
class QString;
class QVariant;

class QDeclarativeDebugStatesDelegate
{
protected:
    QDeclarativeDebugStatesDelegate() {}

public:
    virtual ~QDeclarativeDebugStatesDelegate() {}

    virtual void buildStatesList(QDeclarativeContext *ctxt, bool cleanList) = 0;
    virtual void updateBinding(QDeclarativeContext *context,
                               const QDeclarativeProperty &property,
                               const QVariant &expression, bool isLiteralValue,
                               const QString &fileName, int line,
                               bool *inBaseState) = 0;
    virtual bool setBindingForInvalidProperty(QObject *object,
                                              const QString &propertyName,
                                              const QVariant &expression,
                                              bool isLiteralValue) = 0;
    virtual void resetBindingForInvalidProperty(QObject *object,
                                                const QString &propertyName) = 0;

private:
    Q_DISABLE_COPY(QDeclarativeDebugStatesDelegate)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDECLARATIVEDEBUGSTATESDELEGATE_P_H
