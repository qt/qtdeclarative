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

#ifndef QDECLARATIVEINSTRUCTION_P_H
#define QDECLARATIVEINSTRUCTION_P_H

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

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#define FOR_EACH_QML_INSTR(F) \
    F(Init, init) \
    F(Done, common) \
    F(CreateObject, create) \
    F(CreateSimpleObject, createSimple) \
    F(SetId, setId) \
    F(SetDefault, common) \
    F(CreateComponent, createComponent) \
    F(StoreMetaObject, storeMeta) \
    F(StoreVariant, storeString) \
    F(StoreVariantInteger, storeInteger) \
    F(StoreVariantDouble, storeDouble) \
    F(StoreVariantBool, storeBool) \
    F(StoreString, storeString) \
    F(StoreByteArray, storeByteArray) \
    F(StoreUrl, storeUrl) \
    F(StoreFloat, storeFloat) \
    F(StoreDouble, storeDouble) \
    F(StoreBool, storeBool) \
    F(StoreInteger, storeInteger) \
    F(StoreColor, storeColor) \
    F(StoreDate, storeDate) \
    F(StoreTime, storeTime) \
    F(StoreDateTime, storeDateTime) \
    F(StorePoint, storePoint) \
    F(StorePointF, storePointF) \
    F(StoreSize, storeSize) \
    F(StoreSizeF, storeSizeF) \
    F(StoreRect, storeRect) \
    F(StoreRectF, storeRectF) \
    F(StoreVector3D, storeVector3D) \
    F(StoreObject, storeObject) \
    F(AssignCustomType, assignCustomType) \
    F(AssignSignalObject, assignSignalObject) \
    F(StoreSignal, storeSignal) \
    F(StoreImportedScript, storeScript) \
    F(StoreScriptString, storeScriptString) \
    F(BeginObject, begin) \
    F(InitV8Bindings, initV8Bindings) \
    F(StoreBinding, assignBinding) \
    F(StoreBindingOnAlias, assignBinding) \
    F(StoreV4Binding, assignBinding) \
    F(StoreV8Binding, assignBinding) \
    F(StoreValueSource, assignValueSource) \
    F(StoreValueInterceptor, assignValueInterceptor) \
    F(StoreObjectQList, common) \
    F(AssignObjectList, assignObjectList) \
    F(StoreVariantObject, storeObject) \
    F(StoreInterface, storeObject) \
    F(FetchAttached, fetchAttached) \
    F(FetchQList, fetchQmlList) \
    F(FetchObject, fetch) \
    F(PopQList, common) \
    F(Defer, defer) \
    F(PopFetchedObject, common) \
    F(FetchValueType, fetchValue) \
    F(PopValueType, fetchValue) 

#ifdef Q_ALIGNOF
#  define QML_INSTR_ALIGN_MASK (Q_ALIGNOF(QDeclarativeInstruction) - 1)
#else
#  define QML_INSTR_ALIGN_MASK (sizeof(void *) - 1)
#endif

#define QML_INSTR_HEADER quint8 instructionType;
#define QML_INSTR_ENUM(I, FMT)  I,
#define QML_INSTR_SIZE(I, FMT) ((sizeof(QDeclarativeInstruction::instr_##FMT) + QML_INSTR_ALIGN_MASK) & ~QML_INSTR_ALIGN_MASK)

class QDeclarativeCompiledData;
union QDeclarativeInstruction
{
    enum Type { 
        FOR_EACH_QML_INSTR(QML_INSTR_ENUM)
    };

    inline void setType(Type type) { common.instructionType = type; }
    inline Type type() const { return (Type)common.instructionType; }

    struct instr_common {
        QML_INSTR_HEADER
    };
    struct instr_init {
        QML_INSTR_HEADER
        int bindingsSize;
        int parserStatusSize;
        int contextCache;
        int compiledBinding;
    };
    struct instr_create {
        QML_INSTR_HEADER
        int type;
        int data;
        int bindingBits;
        ushort column;
        ushort line; 
    };
    struct instr_createSimple {
        QML_INSTR_HEADER
        void (*create)(void *);
        int typeSize;
        int type;
        ushort column;
        ushort line; 
    };
    struct instr_storeMeta {
        QML_INSTR_HEADER
        int data;
        int aliasData;
        int propertyCache;
    };
    struct instr_setId {
        QML_INSTR_HEADER
        int value;
        int index;
    };
    struct instr_assignValueSource {
        QML_INSTR_HEADER
        int property;
        int owner;
        int castValue;
    };
    struct instr_assignValueInterceptor {
        QML_INSTR_HEADER
        int property;
        int owner;
        int castValue;
    };
    struct instr_initV8Bindings {
        QML_INSTR_HEADER
        int program;
        ushort programIndex;
        ushort line;
    };
    struct instr_assignBinding {
        QML_INSTR_HEADER
        unsigned int property;
        int value;
        short context;
        short owner;
        ushort line;
    };
    struct instr_fetch {
        QML_INSTR_HEADER
        int property;
        ushort line;
    };
    struct instr_fetchValue {
        QML_INSTR_HEADER
        int property;
        int type;
        quint32 bindingSkipList;
    };
    struct instr_fetchQmlList {
        QML_INSTR_HEADER
        int property;
        int type;
    };
    struct instr_begin {
        QML_INSTR_HEADER
        int castValue;
    }; 
    struct instr_storeFloat {
        QML_INSTR_HEADER
        int propertyIndex;
        float value;
    };
    struct instr_storeDouble {
        QML_INSTR_HEADER
        int propertyIndex;
        double value;
    };
    struct instr_storeInteger {
        QML_INSTR_HEADER
        int propertyIndex;
        int value;
    };
    struct instr_storeBool {
        QML_INSTR_HEADER
        int propertyIndex;
        bool value;
    };
    struct instr_storeString {
        QML_INSTR_HEADER
        int propertyIndex;
        int value;
    };
    struct instr_storeByteArray {
        QML_INSTR_HEADER
        int propertyIndex;
        int value;
    };
    struct instr_storeScriptString {
        QML_INSTR_HEADER
        int propertyIndex;
        int value;
        int scope;
        int bindingId;
        ushort line;
    }; 
    struct instr_storeScript {
        QML_INSTR_HEADER
        int value;
    };
    struct instr_storeUrl {
        QML_INSTR_HEADER
        int propertyIndex;
        int value;
    };
    struct instr_storeColor {
        QML_INSTR_HEADER
        int propertyIndex;
        unsigned int value;
    };
    struct instr_storeDate {
        QML_INSTR_HEADER
        int propertyIndex;
        int value;
    };
    struct instr_storeTime {
        QML_INSTR_HEADER
        int propertyIndex;
        struct QTime {
            int mds;
#if defined(Q_OS_WINCE)
            int startTick;
#endif
        } time;
    };
    struct instr_storeDateTime {
        QML_INSTR_HEADER
        int propertyIndex;
        int date;
        instr_storeTime::QTime time;
    };
    struct instr_storeRect {
        QML_INSTR_HEADER
        int propertyIndex;
        struct QRect {
#if defined(Q_OS_MAC)
            int y1;
            int x1;
            int y2;
            int x2;
#else
            int x1;
            int y1;
            int x2;
            int y2;
#endif
        } rect;
    };
    struct instr_storeRectF {
        QML_INSTR_HEADER
        int propertyIndex;
        struct QRectF {
            qreal xp;
            qreal yp;
            qreal w;
            qreal h;
        } rect;
    };
    struct instr_storeObject {
        QML_INSTR_HEADER
        int propertyIndex;
        ushort line;
    };
    struct instr_assignCustomType {
        QML_INSTR_HEADER
        int propertyIndex;
        int primitive;
        int type;
        ushort line;
    };
    struct instr_storeSignal {
        QML_INSTR_HEADER
        int signalIndex;
        int value;
        short context;
        int name;
        ushort line;
    };
    struct instr_assignSignalObject {
        QML_INSTR_HEADER
        int signal;
        ushort line; 
    };
    struct instr_createComponent {
        QML_INSTR_HEADER
        int count;
        int endLine;
        int metaObject;
        ushort column;
        ushort line;
    };
    struct instr_fetchAttached {
        QML_INSTR_HEADER
        int id;
        ushort line;
    };
    struct instr_defer {
        QML_INSTR_HEADER
        int deferCount;
    };
    struct instr_assignObjectList {
        QML_INSTR_HEADER
        ushort line;
    };
    struct instr_storePoint {
        QML_INSTR_HEADER
        int propertyIndex;
        struct QPoint {
#if defined(Q_OS_MAC)
            int yp;
            int xp;
#else
            int xp;
            int yp;
#endif
        } point;
    };
    struct instr_storePointF {
        QML_INSTR_HEADER
        int propertyIndex;
        struct QPointF {
            qreal xp;
            qreal yp;
        } point;
    };
    struct instr_storeSize {
        QML_INSTR_HEADER
        int propertyIndex;
        struct QSize {
            int wd;
            int ht;
        } size;
    };
    struct instr_storeSizeF {
        QML_INSTR_HEADER
        int propertyIndex;
        struct QSizeF {
            qreal wd;
            qreal ht;
        } size;
    };
    struct instr_storeVector3D {
        QML_INSTR_HEADER
        int propertyIndex;
        struct QVector3D {
            float xp;
            float yp;
            float zp;
        } vector;
    };

    instr_common common;
    instr_init init;
    instr_create create;
    instr_createSimple createSimple;
    instr_storeMeta storeMeta;
    instr_setId setId;
    instr_assignValueSource assignValueSource;
    instr_assignValueInterceptor assignValueInterceptor;
    instr_initV8Bindings initV8Bindings;
    instr_assignBinding assignBinding;
    instr_fetch fetch;
    instr_fetchValue fetchValue;
    instr_fetchQmlList fetchQmlList;
    instr_begin begin;
    instr_storeFloat storeFloat;
    instr_storeDouble storeDouble;
    instr_storeInteger storeInteger;
    instr_storeBool storeBool;
    instr_storeString storeString;
    instr_storeByteArray storeByteArray;
    instr_storeScriptString storeScriptString;
    instr_storeScript storeScript;
    instr_storeUrl storeUrl;
    instr_storeColor storeColor;
    instr_storeDate storeDate;
    instr_storeTime storeTime;
    instr_storeDateTime storeDateTime;
    instr_storePoint storePoint;
    instr_storePointF storePointF;
    instr_storeSize storeSize;
    instr_storeSizeF storeSizeF;
    instr_storeRect storeRect;
    instr_storeRectF storeRectF;
    instr_storeVector3D storeVector3D;
    instr_storeObject storeObject;
    instr_assignCustomType assignCustomType;
    instr_storeSignal storeSignal;
    instr_assignSignalObject assignSignalObject;
    instr_createComponent createComponent;
    instr_fetchAttached fetchAttached;
    instr_defer defer;
    instr_assignObjectList assignObjectList;

    int size() const;
};

template<int N>
struct QDeclarativeInstructionMeta {
};

#define QML_INSTR_META_TEMPLATE(I, FMT) \
    template<> struct QDeclarativeInstructionMeta<(int)QDeclarativeInstruction::I> { \
        enum { Size = QML_INSTR_SIZE(I, FMT) }; \
        typedef QDeclarativeInstruction::instr_##FMT DataType; \
        static const DataType &data(const QDeclarativeInstruction &instr) { return instr.FMT; } \
    }; 
FOR_EACH_QML_INSTR(QML_INSTR_META_TEMPLATE);
#undef QML_INSTR_META_TEMPLATE

QT_END_NAMESPACE

#endif // QDECLARATIVEINSTRUCTION_P_H
