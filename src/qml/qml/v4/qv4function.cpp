/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
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

#include "qv4function_p.h"
#include "qv4managed_p.h"
#include "qv4string_p.h"
#include "qv4value_p.h"

using namespace QV4;

Function::~Function()
{
    delete[] codeData;
}

void Function::mark()
{
    if (name)
        name->mark();
    for (int i = 0; i < formals.size(); ++i)
        formals.at(i)->mark();
    for (int i = 0; i < locals.size(); ++i)
        locals.at(i)->mark();
    for (int i = 0; i < generatedValues.size(); ++i)
        if (Managed *m = generatedValues.at(i).asManaged())
            m->mark();
    for (int i = 0; i < identifiers.size(); ++i)
        identifiers.at(i)->mark();
}

namespace QV4 {
bool operator<(const LineNumberMapping &mapping, quintptr pc)
{
    return mapping.codeOffset < pc;
}
}

int Function::lineNumberForProgramCounter(quintptr pc) const
{
    quintptr offset = pc - reinterpret_cast<quintptr>(code);
    QVector<LineNumberMapping>::ConstIterator it = qLowerBound(lineNumberMappings.begin(), lineNumberMappings.end(), offset);
    if (it != lineNumberMappings.constBegin() && lineNumberMappings.count() > 0)
        --it;
    if (it == lineNumberMappings.constEnd())
        return -1;
    return it->lineNumber;
}

