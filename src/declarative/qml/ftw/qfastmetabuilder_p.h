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

#ifndef QFASTMETABUILDER_P_H
#define QFASTMETABUILDER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of moc.  This header file may change from version to version without notice,
// or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qmetaobject.h>

#include <private/qhashedstring_p.h>

QT_BEGIN_NAMESPACE

struct QMetaObject;
class QFastMetaBuilder
{
public:
    QFastMetaBuilder();
    ~QFastMetaBuilder();

    struct StringRef {
    public:
        inline StringRef();
        inline StringRef(const StringRef &);
        inline StringRef &operator=(const StringRef &);

        inline void load(const QHashedStringRef &);
        inline void load(const QByteArray &);
        inline void load(const char *);

        inline bool isEmpty() const;
        inline QFastMetaBuilder *builder() const;
        inline int offset() const;
        inline char *data();
        inline int length() const;
    private:
        friend class QFastMetaBuilder;

        QFastMetaBuilder *_b;
        int _o;
        int _l;
    };
    StringRef newString(int length);

    // Returns class name
    StringRef init(int classNameLength,
                   int propertyCount, int methodCount, 
                   int signalCount, int classInfoCount);

    void setClassInfo(int index, const StringRef &key, const StringRef &value);

    enum PropertyFlag {
        None = 0x00000000,
        Writable = 0x00000002,
        Resettable = 0x00000004,
        Constant = 0x00000400,
        Final = 0x00000800
    };
    // void setProperty(int index, const StringRef &name, QMetaType::Type type, int notifySignal = -1);
    void setProperty(int index, const StringRef &name, const StringRef &type, 
                     QMetaType::Type mtype, PropertyFlag flags, int notifySignal = -1);
    void setProperty(int index, const StringRef &name, const StringRef &type, 
                     PropertyFlag flags, int notifySignal = -1);
    void setMethod(int index, const StringRef &signature,
                   const StringRef &parameterNames = StringRef(), 
                   const StringRef &type = StringRef());
    void setSignal(int index, const StringRef &signature, 
                   const StringRef &parameterNames = StringRef(), 
                   const StringRef &type = StringRef());

    int metaObjectIndexForSignal(int) const;
    int metaObjectIndexForMethod(int) const;

    QByteArray toData() const { return m_data; }
    static void fromData(QMetaObject *, const QMetaObject *parent, const QByteArray &);
private:
    friend struct StringRef;

    QByteArray m_data;
    int m_zeroPtr;

    void allocateStringData();
    char *m_stringData;
    int m_stringDataLength;
    int m_stringDataAllocated;
};

QFastMetaBuilder::StringRef::StringRef()
: _b(0), _o(0), _l(0)
{
}

QFastMetaBuilder::StringRef::StringRef(const StringRef &o)
: _b(o._b), _o(o._o), _l(o._l)
{
}

QFastMetaBuilder::StringRef &QFastMetaBuilder::StringRef::operator=(const StringRef &o)
{
    _b = o._b;
    _o = o._o;
    _l = o._l;
    return *this;
}

bool QFastMetaBuilder::StringRef::isEmpty() const
{
    return _l == 0;
}

QFastMetaBuilder *QFastMetaBuilder::StringRef::builder() const
{
    return _b;
}

int QFastMetaBuilder::StringRef::offset() const
{
    return _o;
}

char *QFastMetaBuilder::StringRef::data()
{
    Q_ASSERT(_b);
    if (_b->m_stringDataLength != _b->m_stringDataAllocated)
        _b->allocateStringData();
    return _b->m_stringData + _o;
}

int QFastMetaBuilder::StringRef::length() const
{
    return _l;
}

void QFastMetaBuilder::StringRef::load(const QHashedStringRef &str)
{
    Q_ASSERT(str.utf8length() == _l);
    str.writeUtf8(data());
    *(data() + _l) = 0;
}

void QFastMetaBuilder::StringRef::load(const QByteArray &str)
{
    Q_ASSERT(str.length() == _l);
    strcpy(data(), str.constData());
}

void QFastMetaBuilder::StringRef::load(const char *str)
{
    Q_ASSERT(strlen(str) == (uint)_l);
    strcpy(data(), str);
}

QT_END_NAMESPACE

#endif // QFASTMETABUILDER_P_H

