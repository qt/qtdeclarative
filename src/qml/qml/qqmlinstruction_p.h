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

#ifndef QQMLINSTRUCTION_P_H
#define QQMLINSTRUCTION_P_H

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
#include <private/qqmlpropertycache_p.h>

QT_BEGIN_NAMESPACE

#define FOR_EACH_QML_INSTR(F) \
    F(Init, init) \
    F(DeferInit, deferInit) \
    F(Done, common) \
    F(CreateCppObject, create) \
    F(CreateQMLObject, createQml) \
    F(CompleteQMLObject, completeQml) \
    F(CreateSimpleObject, createSimple) \
    F(SetId, setId) \
    F(SetDefault, common) \
    F(CreateComponent, createComponent) \
    F(StoreMetaObject, storeMeta) \
    F(StoreVariant, storeString) \
    F(StoreVariantInteger, storeInteger) \
    F(StoreVariantDouble, storeDouble) \
    F(StoreVariantBool, storeBool) \
    F(StoreVar, storeString) \
    F(StoreVarInteger, storeInteger) \
    F(StoreVarDouble, storeDouble) \
    F(StoreVarBool, storeBool) \
    F(StoreString, storeString) \
    F(StoreJSValueString, storeString) \
    F(StoreJSValueInteger, storeInteger) \
    F(StoreJSValueDouble, storeDouble) \
    F(StoreJSValueBool, storeBool) \
    F(StoreStringList, storeString) \
    F(StoreStringQList, storeString) \
    F_TRANSLATION(F, StoreTrString, storeTrString) \
    F_TRANSLATION(F, StoreTrIdString, storeTrIdString) \
    F(StoreByteArray, storeByteArray) \
    F(StoreUrl, storeUrl) \
    F(StoreUrlQList, storeUrl) \
    F(StoreFloat, storeFloat) \
    F(StoreDouble, storeDouble) \
    F(StoreDoubleQList, storeDouble) \
    F(StoreBool, storeBool) \
    F(StoreBoolQList, storeBool) \
    F(StoreInteger, storeInteger) \
    F(StoreIntegerQList, storeInteger) \
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
    F(StoreVector4D, storeVector4D) \
    F(StoreObject, storeObject) \
    F(AssignCustomType, assignCustomType) \
    F(AssignSignalObject, assignSignalObject) \
    F(StoreSignal, storeSignal) \
    F(StoreImportedScript, storeScript) \
    F(StoreScriptString, storeScriptString) \
    F(BeginObject, begin) \
    F(InitV8Bindings, initV8Bindings) \
    F(StoreBinding, assignBinding) \
    F(StoreV8Binding, assignBinding) \
    F(StoreV4Binding, assignV4Binding) \
    F(StoreValueSource, assignValueSource) \
    F(StoreValueInterceptor, assignValueInterceptor) \
    F(StoreObjectQList, common) \
    F(AssignObjectList, assignObjectList) \
    F(StoreVariantObject, storeObject) \
    F(StoreVarObject, storeObject) \
    F(StoreInterface, storeObject) \
    F(FetchAttached, fetchAttached) \
    F(FetchQList, fetchQmlList) \
    F(FetchObject, fetch) \
    F(PopQList, common) \
    F(Defer, defer) \
    F(PopFetchedObject, common) \
    F(FetchValueType, fetchValue) \
    F(PopValueType, fetchValue) 

#ifndef QT_NO_TRANSLATION
#define F_TRANSLATION(F, I, FMT) F(I, FMT)
#else
#define F_TRANSLATION(F, I, FMT)
#endif

#if defined(Q_CC_GNU) && (!defined(Q_CC_INTEL) || __INTEL_COMPILER >= 1200)
#  define QML_THREADED_VME_INTERPRETER
#endif

#ifdef Q_ALIGNOF
#  define QML_INSTR_ALIGN_MASK (Q_ALIGNOF(QQmlInstruction) - 1)
#else
#  define QML_INSTR_ALIGN_MASK (sizeof(void *) - 1)
#endif

#ifdef QML_THREADED_VME_INTERPRETER
#  define QML_INSTR_HEADER void *code;
#else
#  define QML_INSTR_HEADER quint8 instructionType;
#endif

#define QML_INSTR_ENUM(I, FMT)  I,
#define QML_INSTR_SIZE(I, FMT) ((sizeof(QQmlInstruction::instr_##FMT) + QML_INSTR_ALIGN_MASK) & ~QML_INSTR_ALIGN_MASK)

class QQmlCompiledData;
union QQmlInstruction
{
    enum Type { 
        FOR_EACH_QML_INSTR(QML_INSTR_ENUM)
    };

    struct instr_common {
        QML_INSTR_HEADER
    };
    struct instr_init {
        QML_INSTR_HEADER
        int bindingsSize;
        int parserStatusSize;
        int contextCache;
        int compiledBinding;
        int objectStackSize;
        int listStackSize;
    };
    struct instr_deferInit {
        QML_INSTR_HEADER
        int bindingsSize;
        int parserStatusSize;
        int objectStackSize;
        int listStackSize;
    };
    struct instr_createQml {
        QML_INSTR_HEADER
        int type;
        int bindingBits;
        bool isRoot;
    };
    struct instr_completeQml {
        QML_INSTR_HEADER
        ushort column;
        ushort line; 
        bool isRoot;
    };
    struct instr_create {
        QML_INSTR_HEADER
        int type;
        int data;
        ushort column;
        ushort line; 
        bool isRoot:1;
        bool parentToSuper:1;
    };
    struct instr_createSimple {
        QML_INSTR_HEADER
        void (*create)(void *);
        int typeSize;
        int type;
        ushort column;
        ushort line; 
        bool parentToSuper;
    };
    struct instr_storeMeta {
        QML_INSTR_HEADER
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
        QQmlPropertyRawData property;
        int castValue;
    };
    struct instr_assignValueInterceptor {
        QML_INSTR_HEADER
        QQmlPropertyRawData property;
        int castValue;
    };
    struct instr_initV8Bindings {
        QML_INSTR_HEADER
        ushort programIndex;
        ushort line;
    };
    struct instr_assignV4Binding {
        QML_INSTR_HEADER
        int property;   // ((value type sub-property index << 16) | property index)
        int propType;
        int value;
        int fallbackValue;
        short context;
        short owner;
        bool isRoot:1;
        bool isAlias:1;
        ushort line;
        ushort column;
    };
    struct instr_assignBinding {
        QML_INSTR_HEADER
        QQmlPropertyRawData property;
        int value;
        short context;
        short owner;
        bool isRoot:1;
        bool isAlias:1;
        bool isFallback:1;
        bool isSafe:1;
        ushort line;
        ushort column;
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
    struct instr_storeTrString {
        QML_INSTR_HEADER
        int propertyIndex;
        int context;
        int text;
        int comment;
        int n;
    };
    struct instr_storeTrIdString {
        QML_INSTR_HEADER
        int propertyIndex;
        int text;
        int n;
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
        ushort column;
        double numberValue;
        bool isStringLiteral:1;
        bool isNumberLiteral:1;
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
            int x1;
            int y1;
            int x2;
            int y2;
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
        int parameterCount;
        short context;
        ushort line;
        ushort column;
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
        bool isRoot;
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
            int xp;
            int yp;
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
    struct instr_storeVector4D {
        QML_INSTR_HEADER
        int propertyIndex;
        struct QVector4D {
            float xp;
            float yp;
            float zp;
            float wp;
        } vector;
    };

    instr_common common;
    instr_init init;
    instr_deferInit deferInit;
    instr_create create;
    instr_createQml createQml;
    instr_completeQml completeQml;
    instr_createSimple createSimple;
    instr_storeMeta storeMeta;
    instr_setId setId;
    instr_assignValueSource assignValueSource;
    instr_assignValueInterceptor assignValueInterceptor;
    instr_initV8Bindings initV8Bindings;
    instr_assignV4Binding assignV4Binding;
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
    instr_storeTrString storeTrString;
    instr_storeTrIdString storeTrIdString;
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
    instr_storeVector4D storeVector4D;
    instr_storeObject storeObject;
    instr_assignCustomType assignCustomType;
    instr_storeSignal storeSignal;
    instr_assignSignalObject assignSignalObject;
    instr_createComponent createComponent;
    instr_fetchAttached fetchAttached;
    instr_defer defer;
    instr_assignObjectList assignObjectList;

    static int size(Type type);
};

template<int N>
struct QQmlInstructionMeta {
};

#define QML_INSTR_META_TEMPLATE(I, FMT) \
    template<> struct QQmlInstructionMeta<(int)QQmlInstruction::I> { \
        enum { Size = QML_INSTR_SIZE(I, FMT) }; \
        typedef QQmlInstruction::instr_##FMT DataType; \
        static const DataType &data(const QQmlInstruction &instr) { return instr.FMT; } \
        static void setData(QQmlInstruction &instr, const DataType &v) { instr.FMT = v; } \
    }; 
FOR_EACH_QML_INSTR(QML_INSTR_META_TEMPLATE);
#undef QML_INSTR_META_TEMPLATE

template<int Instr>
class QQmlInstructionData : public QQmlInstructionMeta<Instr>::DataType
{
};

QT_END_NAMESPACE

#endif // QQMLINSTRUCTION_P_H
