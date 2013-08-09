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

#include <qv4unwindhelper_p.h>

#include <wtf/Platform.h>

#if CPU(X86_64) && (OS(LINUX) || OS(MAC_OS_X))
#  define USE_DW2_HELPER
#elif CPU(X86) && COMPILER(GCC)
#  define USE_DW2_HELPER
#elif CPU(ARM) && (OS(LINUX) || OS(QNX))
# define USE_ARM_HELPER
#elif OS(WINDOWS)
    // SJLJ will unwind on Windows
#  define USE_NULL_HELPER
#elif OS(IOS)
    // SJLJ will unwind on iOS
#  define USE_NULL_HELPER
#else
#  warning "Unsupported/untested platform!"
#  define USE_NULL_HELPER
#endif

#ifdef USE_DW2_HELPER
#  include <qv4unwindhelper_p-dw2.h>
#endif // USE_DW2_HELPER

#ifdef USE_ARM_HELPER
#  include <qv4unwindhelper_p-arm.h>
#endif // USE_ARM_HELPER

QT_BEGIN_NAMESPACE

#ifdef USE_NULL_HELPER
using namespace QV4;
void UnwindHelper::prepareForUnwind(ExecutionContext *) {}
void UnwindHelper::registerFunction(Function *function) {Q_UNUSED(function);}
void UnwindHelper::registerFunctions(const QVector<Function *> &functions) {Q_UNUSED(functions);}
void UnwindHelper::deregisterFunction(Function *function) {Q_UNUSED(function);}
void UnwindHelper::deregisterFunctions(const QVector<Function *> &functions) {Q_UNUSED(functions);}
#endif // USE_NULL_HELPER

QT_END_NAMESPACE
