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
#ifndef QV4STRING_H
#define QV4STRING_H

#include <QtCore/qstring.h>
#include <QtCore/QHash>

namespace QQmlJS {
namespace VM {

struct String {
    inline bool isEqualTo(const String *other) const {
        if (this == other)
            return true;
        else if (other && hashValue() == other->hashValue())
            return toQString() == other->toQString();
        return false;
    }

    inline const QString &toQString() const {
        return _text;
    }

    inline unsigned hashValue() const {
        if (_hashValue == InvalidHashValue)
            createHashValue();

        return _hashValue;
    }
    enum {
        InvalidArrayIndex = 0xffffffff,
        LargestHashedArrayIndex = 0x7fffffff,
        InvalidHashValue  = 0xffffffff
    };
    uint asArrayIndex() const {
        if (_hashValue == InvalidHashValue)
            createHashValue();
        if (_hashValue > LargestHashedArrayIndex)
            return InvalidArrayIndex;
        if (_hashValue < LargestHashedArrayIndex)
            return _hashValue;
        return asArrayIndexSlow();
    }
    uint asArrayIndexSlow() const;
    uint toUInt(bool *ok) const;

private:
    friend class StringPool;
    String(const QString &text)
        : _text(text), _hashValue(InvalidHashValue) {}

private:
    void createHashValue() const;
    QString _text;
    mutable unsigned _hashValue;
};

}
}

#endif
