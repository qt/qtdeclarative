/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEUTIL_P_H
#define QDECLARATIVEUTIL_P_H

#include <QtCore/QString>

QT_BEGIN_NAMESPACE

namespace QDeclarativeUtils {

inline bool isUpper(const QChar &qc)
{
    ushort c = qc.unicode();
    return ((c >= 'A' && c <= 'Z') || (c > 127 && QChar::category(c) == QChar::Letter_Uppercase));
}

inline bool isLower(const QChar &qc)
{
    ushort c = qc.unicode();
    return ((c >= 'a' && c <= 'z') || (c > 127 && QChar::category(c) == QChar::Letter_Lowercase));
}

inline bool isLetter(const QChar &qc)
{
    ushort c = qc.unicode();
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c > 127 && qc.isLetter()));
}

inline bool isDigit(const QChar &qc)
{
    ushort c = qc.unicode();
    return ((c >= '0' && c <= '9') || (c > 127 && qc.isDigit()));
}

inline bool isLetterOrNumber(const QChar &qc)
{
    ushort c = qc.unicode();
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c > 127 && qc.isLetterOrNumber()));
}

inline bool isSpace(const QChar &qc)
{
    ushort c = qc.unicode();
    return (c == 0x20 || (c >= 0x09 && c <= 0x0D) || c == 0x85 || (c > 127 && qc.isSpace()));
}

} // namespace QDeclarative

QT_END_NAMESPACE

#endif // QDECLARATIVEUTIL_P_H
