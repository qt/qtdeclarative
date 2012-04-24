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
        inline int index() const;
        inline char *data();
        inline int length() const;
        inline void loadByteArrayData();
    private:
        friend class QFastMetaBuilder;

        QFastMetaBuilder *_b;
        int _i;
        int _o;
        int _l;
    };
    StringRef newString(int length);

    // Returns class name
    StringRef init(int classNameLength,
                   int propertyCount, int methodCount, 
                   int signalCount, int classInfoCount,
                   int paramDataSize, int *paramIndex);

    void setClassInfo(int index, const StringRef &key, const StringRef &value);

    enum PropertyFlag {
        None = 0x00000000,
        Writable = 0x00000002,
        Resettable = 0x00000004,
        Constant = 0x00000400,
        Final = 0x00000800
    };
    void setProperty(int index, const StringRef &name, int type,
                     PropertyFlag flags, int notifySignal = -1);
    void setMethod(int index, const StringRef &name, int paramIndex, int argc = 0,
                   const int *types = 0, const StringRef *parameterNames = 0,
                   QMetaType::Type type = QMetaType::Void);
    void setSignal(int index, const StringRef &name, int paramIndex, int argc = 0,
                   const int *types = 0, const StringRef *parameterNames = 0,
                   QMetaType::Type type = QMetaType::Void);

    int metaObjectIndexForSignal(int) const;
    int metaObjectIndexForMethod(int) const;

    QByteArray toData() const {
        if (m_stringCountLoaded == m_stringCount - 1) {
            // zero-string is lazily loaded last
            const_cast<StringRef &>(m_zeroString).loadByteArrayData();
        }
        Q_ASSERT(m_stringCountLoaded == m_stringCount);
        return m_data;
    }
    static void fromData(QMetaObject *, const QMetaObject *parent, const QByteArray &);
private:
    friend struct StringRef;

    QByteArray m_data;
    StringRef m_zeroString;

    void allocateStringData();
    QByteArrayData *m_stringData;
    int m_stringCount;
    int m_stringDataLength;
    int m_stringCountAllocated;
    int m_stringCountLoaded;
};

QFastMetaBuilder::StringRef::StringRef()
: _b(0), _i(0), _o(0), _l(0)
{
}

QFastMetaBuilder::StringRef::StringRef(const StringRef &o)
: _b(o._b), _i(o._i), _o(o._o), _l(o._l)
{
}

QFastMetaBuilder::StringRef &QFastMetaBuilder::StringRef::operator=(const StringRef &o)
{
    _b = o._b;
    _i = o._i;
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

int QFastMetaBuilder::StringRef::index() const
{
    return _i;
}

char *QFastMetaBuilder::StringRef::data()
{
    Q_ASSERT(_b);
    if (_b->m_stringCountAllocated < _b->m_stringCount)
         _b->allocateStringData();
    return reinterpret_cast<char *>(&_b->m_stringData[_b->m_stringCount]) + _o;
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
    loadByteArrayData();
}

void QFastMetaBuilder::StringRef::load(const QByteArray &str)
{
    Q_ASSERT(str.length() == _l);
    strcpy(data(), str.constData());
    loadByteArrayData();
}

void QFastMetaBuilder::StringRef::load(const char *str)
{
    Q_ASSERT(strlen(str) == (uint)_l);
    strcpy(data(), str);
    loadByteArrayData();
}

void QFastMetaBuilder::StringRef::loadByteArrayData()
{
    if (_b->m_stringCountAllocated < _b->m_stringCount)
         _b->allocateStringData();
    Q_ASSERT(_b->m_stringCountLoaded < _b->m_stringCount);

    int offsetofCstrings = _b->m_stringCount * sizeof(QByteArrayData);
    qptrdiff offset = offsetofCstrings + _o - _i * sizeof(QByteArrayData);

    const QByteArrayData bad = Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(_l, offset);
    memcpy(&_b->m_stringData[_i], &bad, sizeof(QByteArrayData));

    ++_b->m_stringCountLoaded;
}

QT_END_NAMESPACE

#endif // QFASTMETABUILDER_P_H

