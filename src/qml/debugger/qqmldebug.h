/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQMLDEBUG_H
#define QQMLDEBUG_H

#include <QtQml/qtqmlglobal.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


struct Q_QML_EXPORT QQmlDebuggingEnabler
{
    QQmlDebuggingEnabler(bool printWarning = true);
};

// Execute code in constructor before first QQmlEngine is instantiated
#if defined(QT_QML_DEBUG_NO_WARNING)
static QQmlDebuggingEnabler qmlEnableDebuggingHelper(false);
#elif defined(QT_QML_DEBUG) || defined(QT_DECLARATIVE_DEBUG)
static QQmlDebuggingEnabler qmlEnableDebuggingHelper(true);
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QQMLDEBUG_H
