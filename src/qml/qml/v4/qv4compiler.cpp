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

#include "qv4compiler_p.h"
#include "qv4compiler_p_p.h"
#include "qv4program_p.h"
#include "qv4ir_p.h"
#include "qv4irbuilder_p.h"

#include <private/qqmlglobal_p.h>
#include <private/qqmljsast_p.h>
#include <private/qqmlaccessors_p.h>
#include <private/qqmljsengine_p.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(bindingsDump, QML_BINDINGS_DUMP)
DEFINE_BOOL_CONFIG_OPTION(qmlDisableOptimizer, QML_DISABLE_OPTIMIZER)
DEFINE_BOOL_CONFIG_OPTION(qmlVerboseCompiler, QML_VERBOSE_COMPILER)
DEFINE_BOOL_CONFIG_OPTION(qmlBindingsTestEnv, QML_BINDINGS_TEST)

static bool qmlBindingsTest = false;
static bool qmlEnableV4 = true;

using namespace QQmlJS;
QV4CompilerPrivate::QV4CompilerPrivate()
    : subscriptionOffset(0)
    , _function(0) , _block(0) , _discarded(false), registerCount(0)
    , bindingLine(0), bindingColumn(0), invalidatable(false)
{
}

//
// tracing
//
void QV4CompilerPrivate::trace(quint16 line, quint16 column)
{
    bytecode.clear();

    this->bindingLine = line;
    this->bindingColumn = column;
    this->currentReg = _function->tempCount;
    this->registerCount = qMax(this->registerCount, this->currentReg);

    foreach (IR::BasicBlock *bb, _function->basicBlocks) {
        if (! bb->isTerminated() && (bb->index + 1) < _function->basicBlocks.size())
            bb->JUMP(_function->basicBlocks.at(bb->index + 1));
    }

    QVector<IR::BasicBlock *> blocks;
    trace(&blocks);
    currentBlockMask = 0x00000001;


    for (int i = 0; !_discarded && i < blocks.size(); ++i) {
        IR::BasicBlock *block = blocks.at(i);
        IR::BasicBlock *next = i + 1 < blocks.size() ? blocks.at(i + 1) : 0;
        if (IR::Stmt *terminator = block->terminator()) {
            if (IR::CJump *cj = terminator->asCJump()) {
                if (cj->iffalse != next) {
                    IR::Jump *jump = _function->pool->New<IR::Jump>();
                    jump->init(cj->iffalse);
                    block->statements.append(jump);
                }
            } else if (IR::Jump *j = terminator->asJump()) {
                if (j->target == next) {
                    block->statements.resize(block->statements.size() - 1);
                }
            }
        }

        block->offset = bytecode.size();

        if (bytecode.isEmpty()) {
            if (qmlBindingsTest || bindingsDump()) {
                Instr::BindingId id;
                id.column = column;
                id.line = line;
                gen(id);
            }

            if (qmlBindingsTest) {
                QString str = expression->expression.asScript();
                QByteArray strdata((const char *)str.constData(), str.length() * sizeof(QChar));
                int offset = data.count();
                data += strdata;

                Instr::EnableV4Test test;
                test.reg = 0;
                test.offset = offset;
                test.length = str.length();
                gen(test);
            }
        }

        bool usic = false;
        int patchesCount = patches.count();
        qSwap(usedSubscriptionIdsChanged, usic);

        int blockopIndex = bytecode.size();
        Instr::Block blockop;
        blockop.block = currentBlockMask;
        gen(blockop);

        foreach (IR::Stmt *s, block->statements) {
            if (! _discarded)
                s->accept(this);
        }

        qSwap(usedSubscriptionIdsChanged, usic);

        if (usic) {
            if (currentBlockMask == 0x80000000) {
                discard();
                return;
            }
            currentBlockMask <<= 1;
        } else if (! _discarded) {
            const int adjust = bytecode.remove(blockopIndex);
            // Correct patches
            for (int ii = patchesCount; ii < patches.count(); ++ii) 
                patches[ii].offset -= adjust;
        }
    }

#ifdef DEBUG_IR_STRUCTURE
    IR::IRDump dump;
    for (int i = 0; i < blocks.size(); ++i) {
        dump.basicblock(blocks.at(i));
    }
#endif


    if (! _discarded) {
        // back patching
        foreach (const Patch &patch, patches) {
            V4Instr &instr = bytecode[patch.offset];
            int size = V4Instr::size(instructionType(&instr));
            instr.branchop.offset = patch.block->offset - patch.offset - size;
        }

        patches.clear();
    }
}

void QV4CompilerPrivate::trace(QVector<IR::BasicBlock *> *blocks)
{
    for (int i = 0; i < _function->basicBlocks.size(); ++i) {
        IR::BasicBlock *block = _function->basicBlocks.at(i);

        while (! blocks->contains(block)) {
            blocks->append(block);

            if (IR::Stmt *terminator = block->terminator()) {
                if (IR::CJump *cj = terminator->asCJump())
                    block = cj->iffalse;
                else if (IR::Jump *j = terminator->asJump())
                    block = j->target;
            }
        }
    }
}

void QV4CompilerPrivate::traceExpression(IR::Expr *e, quint8 r)
{
    if (!e) {
        discard();
    } else {
        qSwap(currentReg, r);
        e->accept(this);
        qSwap(currentReg, r);
    }
}

//
// expressions
//
void QV4CompilerPrivate::visitConst(IR::Const *e)
{
    switch (e->type) {
    case IR::BoolType: {
        Instr::LoadBool i;
        i.reg = currentReg;
        i.value = e->value;
        gen(i);
        } break;

    case IR::IntType: {
        Instr::LoadInt i;
        i.reg = currentReg;
        i.value = e->value;
        gen(i);
        } break;

    case IR::FloatType:
    case IR::NumberType: {
        Instr::LoadNumber i;
        i.reg = currentReg;
        i.value = e->value;
        gen(i);
        } break;

    case IR::NullType: {
        Instr::LoadNull i;
        i.reg = currentReg;
        gen(i);
        } break;

    default:
        if (qmlVerboseCompiler())
            qWarning() << Q_FUNC_INFO << "unexpected type";
        discard();
    }
}

void QV4CompilerPrivate::visitString(IR::String *e)
{
    registerLiteralString(currentReg, e->value);
}

void QV4CompilerPrivate::visitName(IR::Name *e)
{
    if (e->base) {
        // fetch the object and store it in reg.
        traceExpression(e->base, currentReg);
    } else {
        _subscribeName.clear();
    }

    if (e->storage == IR::Name::RootStorage) {

        Instr::LoadRoot instr;
        instr.reg = currentReg;
        gen(instr);

        if (e->symbol == IR::Name::IdObject) {
            // The ID is a reference to the root object
            return;
        }

    } else if (e->storage == IR::Name::ScopeStorage) {

        Instr::LoadScope instr;
        instr.reg = currentReg;
        gen(instr);

        _subscribeName << contextName();

    } else if (e->storage == IR::Name::IdStorage) {

        Instr::LoadId instr;
        instr.reg = currentReg;
        instr.index = e->idObject->idIndex;
        gen(instr);

        _subscribeName << QLatin1String("$$$ID_") + *e->id;

        if (blockNeedsSubscription(_subscribeName)) {
            Instr::SubscribeId sub;
            sub.reg = currentReg;
            sub.offset = subscriptionIndex(_subscribeName);
            sub.index = instr.index;
            gen(sub);
        }

        return;
    } else {
        // No action needed
    }

    switch (e->symbol) {
    case IR::Name::Unbound: 
    case IR::Name::IdObject: 
    case IR::Name::Slot:
    case IR::Name::Object: {
        Q_ASSERT(!"Unreachable");
        discard();
    } break;

    case IR::Name::AttachType: {
        _subscribeName << *e->id;

        Instr::LoadAttached attached;
        attached.output = currentReg;
        attached.reg = currentReg;
        attached.exceptionId = exceptionId(bindingLine, bindingColumn);
        if (e->declarativeType->attachedPropertiesId() == -1)
            discard();
        attached.id = e->declarativeType->attachedPropertiesId();
        gen(attached);
    } break;

    case IR::Name::SingletonObject: {
        /*
          Existing singleton type object lookup methods include:
              1. string -> singleton object (search via importCache->query(name))
              2. typeid -> singleton object QQmlType (search via ???)
          We currently use 1, which is not ideal for performance
        */
        _subscribeName << *e->id;

        registerLiteralString(currentReg, e->id);

        Instr::LoadSingletonObject module;
        module.reg = currentReg;
        gen(module);
    } break;

    case IR::Name::Property: {
        _subscribeName << *e->id;

        if (e->property->coreIndex == -1) {
            QMetaProperty prop;
            e->property->load(prop, QQmlEnginePrivate::get(engine));
        }

        const int propTy = e->property->propType;
        QQmlRegisterType regType;

        switch (propTy) {
        case QMetaType::Float:
            regType = FloatType;
            break;
        case QMetaType::Double:
            regType = NumberType;
            break;
        case QMetaType::Bool:
            regType = BoolType;
            break;
        case QMetaType::Int:
            regType = IntType;
            break;
        case QMetaType::QString:
            regType = QStringType;
            break;
        case QMetaType::QUrl:
            regType = QUrlType;
            break;
        case QMetaType::QColor:
            regType = QColorType;
            break;
        case QMetaType::QVariant:
            regType = QVariantType;
            break;

        default:
            if (propTy == QQmlMetaType::QQuickAnchorLineMetaTypeId()) {
                regType = PODValueType;
            } else if (!engine->metaObjectForType(propTy).isNull()) {
                regType = QObjectStarType;
            } else {
                if (qmlVerboseCompiler())
                    qWarning() << "Discard unsupported property type:" << QMetaType::typeName(propTy);
                discard(); // Unsupported type
                return;
            }

            break;
        } // switch

        if (e->property->hasAccessors()) {
            Instr::FetchAndSubscribe fetch;
            fetch.reg = currentReg;
            fetch.subscription = subscriptionIndex(_subscribeName);
            fetch.exceptionId = exceptionId(e->line, e->column);
            fetch.valueType = regType;
            fetch.property = *e->property;
            gen(fetch);
        } else {
            Instr::Fetch fetch;
            fetch.reg = currentReg;
            fetch.index = e->property->coreIndex;
            fetch.exceptionId = exceptionId(e->line, e->column);
            fetch.valueType = regType;

            if (blockNeedsSubscription(_subscribeName) && e->property->notifyIndex != -1) {
                fetch.subOffset = subscriptionIndex(_subscribeName);
                fetch.subIndex = e->property->notifyIndex;
            } else {
                fetch.subOffset = static_cast<quint16>(-1);
                fetch.subIndex = static_cast<quint32>(-1);
            }

            gen(fetch);
        }

    } break;
    } // switch
}

void QV4CompilerPrivate::visitTemp(IR::Temp *e)
{
    if (currentReg != e->index) {
        Instr::Copy i;
        i.reg = currentReg;
        i.src = e->index;
        gen(i);
    }
}

void QV4CompilerPrivate::visitUnop(IR::Unop *e)
{
    quint8 src = currentReg;
    
    if (IR::Temp *temp = e->expr->asTemp()) {
        src = temp->index;
    } else {
        traceExpression(e->expr, src);
    }

    switch (e->op) {
    case IR::OpInvalid:
        Q_ASSERT(!"unreachable");
        break;

    case IR::OpIfTrue:
        convertToBool(e->expr, src);
        if (src != currentReg) {
            Instr::Copy i;
            i.reg = currentReg;
            i.src = src;
            gen(i);
        }
        break;

    case IR::OpNot: {
        Instr::UnaryNot i;
        convertToBool(e->expr, src);
        i.output = currentReg;
        i.src = src;
        gen(i);
        } break;

    case IR::OpUMinus:
        if (IR::isRealType(e->expr->type)) {
            Instr::UnaryMinusNumber i;
            i.output = currentReg;
            i.src = src;
            gen(i);
        } else if (e->expr->type == IR::IntType) {
            convertToNumber(e->expr, currentReg);
            Instr::UnaryMinusNumber i;
            i.output = currentReg;
            i.src = src;
            gen(i);
        } else {
            discard();
        }
        break;

    case IR::OpUPlus:
        if (IR::isRealType(e->expr->type)) {
            Instr::UnaryPlusNumber i;
            i.output = currentReg;
            i.src = src;
            gen(i);
        } else if (e->expr->type == IR::IntType) {
            convertToNumber(e->expr, currentReg);
            Instr::UnaryPlusNumber i;
            i.output = currentReg;
            i.src = src;
            gen(i);
        } else {
            discard();
        }
        break;

    case IR::OpCompl:
        // TODO
        discard();
        break;

    case IR::OpBitAnd:
    case IR::OpBitOr:
    case IR::OpBitXor:
    case IR::OpAdd:
    case IR::OpSub:
    case IR::OpMul:
    case IR::OpDiv:
    case IR::OpMod:
    case IR::OpLShift:
    case IR::OpRShift:
    case IR::OpURShift:
    case IR::OpGt:
    case IR::OpLt:
    case IR::OpGe:
    case IR::OpLe:
    case IR::OpEqual:
    case IR::OpNotEqual:
    case IR::OpStrictEqual:
    case IR::OpStrictNotEqual:
    case IR::OpAnd:
    case IR::OpOr:
        Q_ASSERT(!"unreachable");
        break;
    } // switch
}

void QV4CompilerPrivate::convertToNumber(IR::Expr *expr, int reg)
{
    if (expr->type == IR::NumberType)
        return;

    switch (expr->type) {
    case IR::BoolType: {
        Instr::ConvertBoolToNumber i;
        i.output = i.src = reg;
        gen(i);
        } break;

    case IR::IntType: {
        Instr::ConvertIntToNumber i;
        i.output = i.src = reg;
        gen(i);
        } break;

    case IR::FloatType:
    case IR::NumberType:
        // nothing to do
        return;

    default:
        discard();
        break;
    } // switch
}

void QV4CompilerPrivate::convertToInt(IR::Expr *expr, int reg)
{
    if (expr->type == IR::IntType)
        return;

    switch (expr->type) {
    case IR::BoolType: {
        Instr::ConvertBoolToInt i;
        i.output = i.src = reg;
        gen(i);
        } break;

    case IR::IntType:
        // nothing to do
        return;

    case IR::FloatType:
    case IR::NumberType: {
        Instr::ConvertNumberToInt i;
        i.output = i.src = reg;
        gen(i);
        } break;

    default:
        discard();
        break;
    } // switch
}

void QV4CompilerPrivate::convertToBool(IR::Expr *expr, int reg)
{
    if (expr->type == IR::BoolType)
        return;

    switch (expr->type) {
    case IR::BoolType:
        // nothing to do
        break;

    case IR::IntType: {
        Instr::ConvertIntToBool i;
        i.output = i.src = reg;
        gen(i);
        } break;

    case IR::FloatType:
    case IR::NumberType: {
        Instr::ConvertNumberToBool i;
        i.output = i.src = reg;
        gen(i);
        } return;

    case IR::StringType: {
        Instr::ConvertStringToBool i;
        i.output = i.src = reg;
        gen(i);
        } return;

    case IR::ColorType: {
        Instr::ConvertColorToBool i;
        i.output = i.src = reg;
        gen(i);
        } return;

    default:
        discard();
        break;
    } // switch
}

quint8 QV4CompilerPrivate::instructionOpcode(IR::Binop *e)
{
    switch (e->op) {
    case IR::OpInvalid:
        return V4Instr::Noop;

    case IR::OpIfTrue:
    case IR::OpNot:
    case IR::OpUMinus:
    case IR::OpUPlus:
    case IR::OpCompl:
        return V4Instr::Noop;

    case IR::OpBitAnd:
        return V4Instr::BitAndInt;

    case IR::OpBitOr:
        return V4Instr::BitOrInt;

    case IR::OpBitXor:
        return V4Instr::BitXorInt;

    case IR::OpAdd:
        if (e->type == IR::StringType)
            return V4Instr::AddString;
        return V4Instr::AddNumber;

    case IR::OpSub:
        return V4Instr::SubNumber;

    case IR::OpMul:
        return V4Instr::MulNumber;

    case IR::OpDiv:
        return V4Instr::DivNumber;

    case IR::OpMod:
        return V4Instr::ModNumber;

    case IR::OpLShift:
        return V4Instr::LShiftInt;

    case IR::OpRShift:
        return V4Instr::RShiftInt;

    case IR::OpURShift:
        return V4Instr::URShiftInt;

    case IR::OpGt:
        if (e->left->type == IR::StringType)
            return V4Instr::GtString;
        return V4Instr::GtNumber;

    case IR::OpLt:
        if (e->left->type == IR::StringType)
            return V4Instr::LtString;
        return V4Instr::LtNumber;

    case IR::OpGe:
        if (e->left->type == IR::StringType)
            return V4Instr::GeString;
        return V4Instr::GeNumber;

    case IR::OpLe:
        if (e->left->type == IR::StringType)
            return V4Instr::LeString;
        return V4Instr::LeNumber;

    case IR::OpEqual:
        if (e->left->type == IR::ObjectType || e->right->type == IR::ObjectType)
            return V4Instr::EqualObject;
        if (e->left->type == IR::StringType)
            return V4Instr::EqualString;
        return V4Instr::EqualNumber;

    case IR::OpNotEqual:
        if (e->left->type == IR::ObjectType || e->right->type == IR::ObjectType)
            return V4Instr::NotEqualObject;
        if (e->left->type == IR::StringType)
            return V4Instr::NotEqualString;
        return V4Instr::NotEqualNumber;

    case IR::OpStrictEqual:
        if (e->left->type == IR::ObjectType || e->right->type == IR::ObjectType)
            return V4Instr::StrictEqualObject;
        if (e->left->type == IR::StringType)
            return V4Instr::StrictEqualString;
        return V4Instr::StrictEqualNumber;

    case IR::OpStrictNotEqual:
        if (e->left->type == IR::ObjectType || e->right->type == IR::ObjectType)
            return V4Instr::StrictNotEqualObject;
        if (e->left->type == IR::StringType)
            return V4Instr::StrictNotEqualString;
        return V4Instr::StrictNotEqualNumber;

    case IR::OpAnd:
    case IR::OpOr:
        return V4Instr::Noop;

    } // switch

    return V4Instr::Noop;
}

void QV4CompilerPrivate::visitBinop(IR::Binop *e)
{
    if (e->type == IR::InvalidType) {
        discard();
        return;
    }

    int left = currentReg;
    int right = currentReg + 1; 

    if (e->left->asTemp() && e->type != IR::StringType)
        left = e->left->asTemp()->index;
    else
        traceExpression(e->left, left);

    if (IR::Temp *t = e->right->asTemp())
        right = t->index;
    else
        traceExpression(e->right, right);

    if (e->left->type != e->right->type) {
        if (qmlVerboseCompiler())
            qWarning().nospace() << "invalid operands to binary operator " << IR::binaryOperator(e->op)
                                 << "(`" << IR::binaryOperator(e->left->type)
                                 << "' and `"
                                 << IR::binaryOperator(e->right->type)
                                 << '\'';
        discard();
        return;
    }

    switch (e->op) {
    case IR::OpInvalid:
        discard();
        break;

    // unary
    case IR::OpIfTrue:
    case IR::OpNot:
    case IR::OpUMinus:
    case IR::OpUPlus:
    case IR::OpCompl:
        discard();
        break;

    case IR::OpBitAnd:
    case IR::OpBitOr:
    case IR::OpBitXor:
    case IR::OpLShift:
    case IR::OpRShift:
    case IR::OpURShift:
        convertToInt(e->left, left);
        convertToInt(e->right, right);
        break;

    case IR::OpAdd:
        if (e->type != IR::StringType) {
            convertToNumber(e->left, left);
            convertToNumber(e->right, right);
        }
        break;

    case IR::OpSub:
    case IR::OpMul:
    case IR::OpDiv:
    case IR::OpMod:
        convertToNumber(e->left, left);
        convertToNumber(e->right, right);
        break;

    case IR::OpGt:
    case IR::OpLt:
    case IR::OpGe:
    case IR::OpLe:
    case IR::OpEqual:
    case IR::OpNotEqual:
    case IR::OpStrictEqual:
    case IR::OpStrictNotEqual:
        if (e->left->type >= IR::FirstNumberType) {
            convertToNumber(e->left, left);
            convertToNumber(e->right, right);
        }
        break;

    case IR::OpAnd:
    case IR::OpOr:
        discard(); // ### unreachable
        break;
    } // switch

    const quint8 opcode = instructionOpcode(e);
    if (opcode != V4Instr::Noop) {
        V4Instr instr;
        instr.binaryop.output = currentReg;
        instr.binaryop.left = left;
        instr.binaryop.right = right;
        gen(static_cast<V4Instr::Type>(opcode), instr);
    }
}

void QV4CompilerPrivate::visitCall(IR::Call *call)
{
    if (IR::Name *name = call->base->asName()) {
        IR::Expr *arg = call->onlyArgument();
        if (arg != 0 && IR::isRealType(arg->type)) {
            traceExpression(arg, currentReg);

            switch (name->builtin) {
            case IR::NoBuiltinSymbol:
                break;

            case IR::MathSinBultinFunction: {
                Instr::MathSinNumber i;
                i.output = i.src = currentReg;
                gen(i);
                } return;

            case IR::MathCosBultinFunction: {
                Instr::MathCosNumber i;
                i.output = i.src = currentReg;
                gen(i);
                } return;

            case IR::MathAbsBuiltinFunction: {
                Instr::MathAbsNumber i;
                i.output = i.src = currentReg;
                gen(i);
                } return;

            case IR::MathRoundBultinFunction: {
                Instr::MathRoundNumber i;
                i.output = i.src = currentReg;
                gen(i);
                } return;

            case IR::MathFloorBultinFunction: {
                Instr::MathFloorNumber i;
                i.output = i.src = currentReg;
                gen(i);
                } return;

            case IR::MathCeilBuiltinFunction: {
                Instr::MathCeilNumber i;
                i.output = i.src = currentReg;
                gen(i);
                } return;

            case IR::MathPIBuiltinConstant:
            default:
                break;
            } // switch
        } else {
            if (name->builtin == IR::MathMaxBuiltinFunction ||
                name->builtin == IR::MathMinBuiltinFunction) {

                //only handles the most common case of exactly two arguments
                if (call->args && call->args->next && !call->args->next->next) {
                    IR::Expr *arg1 = call->args->expr;
                    IR::Expr *arg2 = call->args->next->expr;

                    if (arg1 != 0 && IR::isRealType(arg1->type) &&
                        arg2 != 0 && IR::isRealType(arg2->type)) {

                        traceExpression(arg1, currentReg);
                        traceExpression(arg2, currentReg + 1);

                        if (name->builtin == IR::MathMaxBuiltinFunction) {
                            Instr::MathMaxNumber i;
                            i.left = currentReg;
                            i.right = currentReg + 1;
                            i.output = currentReg;
                            gen(i);
                            return;
                        } else if (name->builtin == IR::MathMinBuiltinFunction) {
                            Instr::MathMinNumber i;
                            i.left = currentReg;
                            i.right = currentReg + 1;
                            i.output = currentReg;
                            gen(i);
                            return;
                        }
                    }
                }
            }
        }
    }

    if (qmlVerboseCompiler())
        qWarning() << "TODO:" << Q_FUNC_INFO << __LINE__;
    discard();
}


//
// statements
//
void QV4CompilerPrivate::visitExp(IR::Exp *s)
{
    traceExpression(s->expr, currentReg);
}

void QV4CompilerPrivate::visitMove(IR::Move *s)
{
    IR::Temp *target = s->target->asTemp();
    Q_ASSERT(target != 0);

    quint8 dest = target->index;

    IR::Type targetTy = s->target->type;
    IR::Type sourceTy = s->source->type;

    // promote the floats
    if (sourceTy == IR::FloatType)
        sourceTy = IR::NumberType;

    if (targetTy == IR::FloatType)
        targetTy = IR::NumberType;

    if (sourceTy != targetTy) {
        quint8 src = dest;

        if (IR::Temp *t = s->source->asTemp()) 
            src = t->index;
        else
            traceExpression(s->source, dest);

        V4Instr::Type opcode = V4Instr::Noop;

        if (sourceTy == IR::UrlType) {
            switch (targetTy) {
            case IR::BoolType:
            case IR::StringType:
            case IR::VariantType:
            case IR::VarType:
            case IR::JSValueType:
                // nothing to do. V4 will generate optimized
                // url-to-xxx conversions.
                break;
            default: {
                if (s->isMoveForReturn) {
                    V4Instr instr;
                    instr.throwop.exceptionId = exceptionId(bindingLine, bindingColumn);
                    registerLiteralString(dest, _function->newString(QString::fromUtf8("Unable to assign %1 to %2")
                                                                     .arg(QLatin1String(IR::typeName(sourceTy)))
                                                                     .arg(QLatin1String(IR::typeName(targetTy)))));
                    instr.throwop.message = dest;
                    gen(V4Instr::Throw, instr);
                    return;
                }
                // generate a UrlToString conversion and fix
                // the type of the source expression.
                V4Instr conv;
                conv.unaryop.output = src;
                conv.unaryop.src = src;
                gen(V4Instr::ConvertUrlToString, conv);
                sourceTy = IR::StringType;
                break;
            }
            } // switch
        }

        if (targetTy == IR::BoolType) {
            switch (sourceTy) {
            case IR::IntType: opcode = V4Instr::ConvertIntToBool; break;
            case IR::NumberType: opcode = V4Instr::ConvertNumberToBool; break;
            case IR::StringType: opcode = V4Instr::ConvertStringToBool; break;
            case IR::UrlType: opcode = V4Instr::ConvertUrlToBool; break;
            case IR::ColorType: opcode = V4Instr::ConvertColorToBool; break;
            case IR::ObjectType: opcode = V4Instr::ConvertObjectToBool; break;
            default: break;
            } // switch
        } else if (targetTy == IR::IntType) {
            switch (sourceTy) {
            case IR::BoolType: opcode = V4Instr::ConvertBoolToInt; break;
            case IR::NumberType: {
                if (s->isMoveForReturn)
                    opcode = V4Instr::MathRoundNumber;
                else
                    opcode = V4Instr::ConvertNumberToInt;
                break;
            }
            case IR::StringType: opcode = V4Instr::ConvertStringToInt; break;
            default: break;
            } // switch
        } else if (IR::isRealType(targetTy)) {
            switch (sourceTy) {
            case IR::BoolType: opcode = V4Instr::ConvertBoolToNumber; break;
            case IR::IntType: opcode = V4Instr::ConvertIntToNumber; break;
            case IR::StringType: opcode = V4Instr::ConvertStringToNumber; break;
            default: break;
            } // switch
        } else if (targetTy == IR::StringType) {
            switch (sourceTy) {
            case IR::BoolType: opcode = V4Instr::ConvertBoolToString; break;
            case IR::IntType:  opcode = V4Instr::ConvertIntToString; break;
            case IR::NumberType: opcode = V4Instr::ConvertNumberToString; break;
            case IR::UrlType: opcode = V4Instr::ConvertUrlToString; break;
            case IR::ColorType: opcode = V4Instr::ConvertColorToString; break;
            default: break;
            } // switch
        } else if (targetTy == IR::UrlType) {
            if (s->isMoveForReturn && sourceTy != IR::StringType) {
                V4Instr instr;
                instr.throwop.exceptionId = exceptionId(bindingLine, bindingColumn);
                registerLiteralString(dest, _function->newString(QString::fromUtf8("Unable to assign %1 to %2")
                                                                 .arg(QLatin1String(IR::typeName(sourceTy)))
                                                                 .arg(QLatin1String(IR::typeName(targetTy)))));
                instr.throwop.message = dest;
                gen(V4Instr::Throw, instr);
                return;
            }

            V4Instr convToString;
            convToString.unaryop.output = dest;
            convToString.unaryop.src = src;

            // try to convert the source expression to a string.
            switch (sourceTy) {
            case IR::BoolType: gen(V4Instr::ConvertBoolToString, convToString); sourceTy = IR::StringType; break;
            case IR::IntType:  gen(V4Instr::ConvertIntToString,  convToString); sourceTy = IR::StringType; break;
            case IR::NumberType: gen(V4Instr::ConvertNumberToString, convToString); sourceTy = IR::StringType; break;
            case IR::ColorType: gen(V4Instr::ConvertColorToString, convToString); sourceTy = IR::StringType; break;
            default: break;
            } // switch

            if (sourceTy == IR::StringType)
                opcode = V4Instr::ConvertStringToUrl;
        } else if (targetTy == IR::ColorType) {
            switch (sourceTy) {
            case IR::StringType: opcode = V4Instr::ConvertStringToColor; break;
            default: break;
            } // switch
        } else if (targetTy == IR::ObjectType) {
            switch (sourceTy) {
            case IR::NullType: opcode = V4Instr::ConvertNullToObject; break;
            default: break;
            } // switch
        } else if (targetTy == IR::VariantType) {
            if (s->isMoveForReturn) {
                switch (sourceTy) {
                case IR::BoolType: opcode = V4Instr::ConvertBoolToVariant; break;
                case IR::IntType:  opcode = V4Instr::ConvertIntToVariant; break;
                case IR::NumberType: opcode = V4Instr::ConvertNumberToVariant; break;
                case IR::UrlType: opcode = V4Instr::ConvertUrlToVariant; break;
                case IR::ColorType: opcode = V4Instr::ConvertColorToVariant; break;
                case IR::StringType: opcode = V4Instr::ConvertStringToVariant; break;
                case IR::ObjectType: opcode = V4Instr::ConvertObjectToVariant; break;
                case IR::NullType: opcode = V4Instr::ConvertNullToVariant; break;
                default: break;
                } // switch
            }
        } else if (targetTy == IR::VarType) {
            if (s->isMoveForReturn) {
                switch (sourceTy) {
                case IR::BoolType: opcode = V4Instr::ConvertBoolToVar; break;
                case IR::IntType:  opcode = V4Instr::ConvertIntToVar; break;
                case IR::NumberType: opcode = V4Instr::ConvertNumberToVar; break;
                case IR::UrlType: opcode = V4Instr::ConvertUrlToVar; break;
                case IR::ColorType: opcode = V4Instr::ConvertColorToVar; break;
                case IR::StringType: opcode = V4Instr::ConvertStringToVar; break;
                case IR::ObjectType: opcode = V4Instr::ConvertObjectToVar; break;
                case IR::NullType: opcode = V4Instr::ConvertNullToVar; break;
                case IR::JSValueType: opcode = V4Instr::ConvertJSValueToVar; break;
                default: break;
                } // switch
            }
        } else if (targetTy == IR::JSValueType) {
            if (s->isMoveForReturn) {
                switch (sourceTy) {
                case IR::BoolType: opcode = V4Instr::ConvertBoolToJSValue; break;
                case IR::IntType:  opcode = V4Instr::ConvertIntToJSValue; break;
                case IR::NumberType: opcode = V4Instr::ConvertNumberToJSValue; break;
                case IR::UrlType: opcode = V4Instr::ConvertUrlToJSValue; break;
                case IR::ColorType: opcode = V4Instr::ConvertColorToJSValue; break;
                case IR::StringType: opcode = V4Instr::ConvertStringToJSValue; break;
                case IR::ObjectType: opcode = V4Instr::ConvertObjectToJSValue; break;
                case IR::VarType: opcode = V4Instr::ConvertVarToJSValue; break;
                case IR::NullType: opcode = V4Instr::ConvertNullToJSValue; break;
                default: break;
                }
            }
        }
        if (opcode != V4Instr::Noop) {
            V4Instr conv;
            conv.unaryop.output = dest;
            conv.unaryop.src = src;
            gen(opcode, conv);

            if (s->isMoveForReturn && opcode == V4Instr::ConvertStringToUrl) {
                V4Instr resolveUrl;
                resolveUrl.unaryop.output = dest;
                resolveUrl.unaryop.src = dest;
                gen(V4Instr::ResolveUrl, resolveUrl);
            }
        } else {
            discard();
        }
    } else {
        traceExpression(s->source, dest);
    }
}

void QV4CompilerPrivate::visitJump(IR::Jump *s)
{
    patches.append(Patch(s->target, bytecode.size()));

    Instr::Branch i;
    i.offset = 0; // ### backpatch
    gen(i);
}

void QV4CompilerPrivate::visitCJump(IR::CJump *s)
{
    traceExpression(s->cond, currentReg);

    patches.append(Patch(s->iftrue, bytecode.size()));

    Instr::BranchTrue i;
    i.reg = currentReg;
    i.offset = 0; // ### backpatch
    gen(i);
}

void QV4CompilerPrivate::visitRet(IR::Ret *s)
{
    Q_ASSERT(s->expr != 0);

    int storeReg = currentReg;

    if (IR::Temp *temp = s->expr->asTemp()) {
        storeReg = temp->index;
    } else {
        traceExpression(s->expr, storeReg);
    }

    if (qmlBindingsTest) {
        Instr::TestV4Store test;
        test.reg = storeReg;
        switch (s->type) {
        case IR::StringType:
            test.regType = QMetaType::QString;
            break;
        case IR::UrlType:
            test.regType = QMetaType::QUrl;
            break;
        case IR::ColorType:
            test.regType = QMetaType::QColor;
            break;
        case IR::SGAnchorLineType:
            test.regType = QQmlMetaType::QQuickAnchorLineMetaTypeId();
            break;
        case IR::ObjectType:
            test.regType = QMetaType::QObjectStar;
            break;
        case IR::VariantType:
            test.regType = QMetaType::QVariant;
            break;
        case IR::VarType:
            test.regType = qMetaTypeId<v8::Handle<v8::Value> >();
            break;
        case IR::JSValueType:
            test.regType = qMetaTypeId<QJSValue>();
            break;
        case IR::BoolType:
            test.regType = QMetaType::Bool;
            break;
        case IR::IntType:
            test.regType = QMetaType::Int;
            break;
        case IR::FloatType:
        case IR::NumberType:
            test.regType = QMetaType::Double;
            break;
        default:
            discard();
            return;
        }
        gen(test);
    }

    Instr::Store store;
    store.output = 0;
    store.index = expression->property->index;
    store.reg = storeReg;
    store.valueType = s->type == IR::FloatType ? FloatType : 0;
    store.exceptionId = exceptionId(s->line, s->column);
    gen(store);
}

void QV4Compiler::dump(const QByteArray &programData)
{
    const QV4Program *program = (const QV4Program *)programData.constData();

    qWarning() << "Program.bindings:" << program->bindings;
    qWarning() << "Program.dataLength:" << program->dataLength;
    qWarning() << "Program.subscriptions:" << program->subscriptions;

    const int programSize = program->instructionCount;
    const char *start = program->instructions();
    const char *end = start + programSize;
    Bytecode bc;
    bc.dump(start, end);
}

/*!
Clear the state associated with attempting to compile a specific binding.
This does not clear the global "committed binding" states.
*/
void QV4CompilerPrivate::resetInstanceState()
{
    data = committed.data;
    exceptions = committed.exceptions;
    usedSubscriptionIds.clear();
    subscriptionIds.clear();
    subscriptionOffset = committed.subscriptionCount;
    bytecode.clear();
    patches.clear();
    pool.clear();
    currentReg = 0;
    invalidatable = false;
}

/*!
Mark the last compile as successful, and add it to the "committed data"
section.

Returns the index for the committed binding.
*/
int QV4CompilerPrivate::commitCompile()
{
    int rv = committed.count();
    committed.offsets << committed.bytecode.count();
    committed.dependencies << usedSubscriptionIds;
    committed.bytecode.append(bytecode.constData(), bytecode.size());
    committed.data = data;
    committed.exceptions = exceptions;
    committed.subscriptionCount = subscriptionOffset + subscriptionIds.count();
    if (bindingsDump())
        committed.subscriptions.append(subscriptionIds);
    return rv;
}

bool QV4CompilerPrivate::compile(QQmlJS::AST::Node *node)
{
    resetInstanceState();

    if (expression->property->type == -1)
        return false;

    AST::SourceLocation location;
    if (AST::ExpressionNode *astExpression = node->expressionCast()) {
        location = astExpression->firstSourceLocation();
    } else if (AST::Statement *astStatement = node->statementCast()) {
        if (AST::Block *block = AST::cast<AST::Block *>(astStatement))
            location = block->lbraceToken;
        else if (AST::IfStatement *ifStmt = AST::cast<AST::IfStatement *>(astStatement))
            location = ifStmt->ifToken;
        else
            return false;
    } else {
        return false;
    }

    IR::Function thisFunction(&pool), *function = &thisFunction;

    QV4IRBuilder irBuilder(expression, engine);
    if (!irBuilder(function, node, &invalidatable))
        return false;

    bool discarded = false;
    qSwap(_discarded, discarded);
    qSwap(_function, function);
    trace(location.startLine, location.startColumn);
    qSwap(_function, function);
    qSwap(_discarded, discarded);

    if (qmlVerboseCompiler()) {
        QTextStream qerr(stderr, QIODevice::WriteOnly);
        if (discarded)
            qerr << "======== TODO ====== " << endl;
        else 
            qerr << "==================== " << endl;
        qerr << "\tline: " << location.startLine
             << "\tcolumn: " << location.startColumn
             << endl;
        foreach (IR::BasicBlock *bb, function->basicBlocks)
            bb->dump(qerr);
        qerr << endl;
    }

    if (discarded || subscriptionIds.count() > 0xFFFF || registerCount > 31)
        return false;

    return true;
}

// Returns a reg
int QV4CompilerPrivate::registerLiteralString(quint8 reg, const QStringRef &str)
{
    // ### string cleanup

    QByteArray strdata((const char *)str.constData(), str.length() * sizeof(QChar));
    int offset = data.count();
    data += strdata;

    Instr::LoadString string;
    string.reg = reg;
    string.offset = offset;
    string.length = str.length();
    gen(string);

    return reg;
}

/*!
Returns true if the current expression has not already subscribed to \a sub in currentBlockMask.
*/
bool QV4CompilerPrivate::blockNeedsSubscription(const QStringList &sub)
{
    QString str = sub.join(QLatin1String("."));

    int *iter = subscriptionIds.value(str);
    if (!iter)
        return true;

    quint32 *uiter = usedSubscriptionIds.value(*iter);
    if (!uiter)
        return true;
    else
        return !(*uiter & currentBlockMask);
}

int QV4CompilerPrivate::subscriptionIndex(const QStringList &sub)
{
    QString str = sub.join(QLatin1String("."));
    int *iter = subscriptionIds.value(str);
    if (!iter) {
        int count = subscriptionOffset + subscriptionIds.count();
        iter = &subscriptionIds[str];
        *iter = count;
    }
    quint32 &u = usedSubscriptionIds[*iter];
    if (!(u & currentBlockMask)) {
        u |= currentBlockMask;
        usedSubscriptionIdsChanged = true;
    }
    return *iter;
}

quint32 QV4CompilerPrivate::subscriptionBlockMask(const QStringList &sub)
{
    QString str = sub.join(QLatin1String("."));

    int *iter = subscriptionIds.value(str);
    Q_ASSERT(iter != 0);

    quint32 *uiter = usedSubscriptionIds.value(*iter);
    Q_ASSERT(uiter != 0);

    return *uiter;
}

quint8 QV4CompilerPrivate::exceptionId(quint16 line, quint16 column)
{
    quint8 rv = 0xFF;
    if (exceptions.count() < 0xFF) {
        rv = (quint8)exceptions.count();
        quint32 e = line;
        e <<= 16;
        e |= column;
        exceptions.append(e);
    }
    return rv;
}

quint8 QV4CompilerPrivate::exceptionId(QQmlJS::AST::ExpressionNode *n)
{
    quint8 rv = 0xFF;
    if (n && exceptions.count() < 0xFF) {
        QQmlJS::AST::SourceLocation l = n->firstSourceLocation();
        rv = exceptionId(l.startLine, l.startColumn);
    }
    return rv;
}

QV4Compiler::QV4Compiler()
: d(new QV4CompilerPrivate)
{
    qmlBindingsTest |= qmlBindingsTestEnv();
}

QV4Compiler::~QV4Compiler()
{
    delete d; d = 0;
}

/* 
Returns true if any bindings were compiled.
*/
bool QV4Compiler::isValid() const
{
    return !d->committed.bytecode.isEmpty();
}

/* 
-1 on failure, otherwise the binding index to use.
*/
int QV4Compiler::compile(const Expression &expression, QQmlEnginePrivate *engine, bool *invalidatable)
{
    if (!expression.expression.asAST()) return false;

    if (qmlDisableOptimizer() || !qmlEnableV4)
        return -1;

    d->expression = &expression;
    d->engine = engine;

    if (d->compile(expression.expression.asAST())) {
        *invalidatable = d->isInvalidatable();
        return d->commitCompile();
    } else {
        return -1;
    }
}

QByteArray QV4CompilerPrivate::buildSignalTable() const
{
    QHash<int, QList<QPair<int, quint32> > > table;

    for (int ii = 0; ii < committed.count(); ++ii) {
        const QQmlAssociationList<int, quint32> &deps = committed.dependencies.at(ii);
        for (QQmlAssociationList<int, quint32>::const_iterator iter = deps.begin(); iter != deps.end(); ++iter)
            table[iter->first].append(qMakePair(ii, iter->second));
    }

    QVector<quint32> header;
    QVector<quint32> data;
    for (int ii = 0; ii < committed.subscriptionCount; ++ii) {
        header.append(committed.subscriptionCount + data.count());
        const QList<QPair<int, quint32> > &bindings = table[ii];
        data.append(bindings.count());
        for (int jj = 0; jj < bindings.count(); ++jj) {
            data.append(bindings.at(jj).first);
            data.append(bindings.at(jj).second);
        }
    }
    header << data;

    return QByteArray((const char *)header.constData(), header.count() * sizeof(quint32));
}

QByteArray QV4CompilerPrivate::buildExceptionData() const
{
    QByteArray rv;
    rv.resize(committed.exceptions.count() * sizeof(quint32));
    ::memcpy(rv.data(), committed.exceptions.constData(), rv.size());
    return rv;
}

/* 
Returns the compiled program.
*/
QByteArray QV4Compiler::program() const
{
    QByteArray programData;

    if (isValid()) {
        QV4Program prog;
        prog.bindings = d->committed.count();

        Bytecode bc;
        QV4CompilerPrivate::Instr::Jump jump;
        jump.reg = -1;

        for (int ii = 0; ii < d->committed.count(); ++ii) {
            jump.count = d->committed.count() - ii - 1;
            jump.count*= V4InstrMeta<V4Instr::Jump>::Size;
            jump.count+= d->committed.offsets.at(ii);
            bc.append(jump);
        }


        QByteArray bytecode;
        bytecode.reserve(bc.size() + d->committed.bytecode.size());
        bytecode.append(bc.constData(), bc.size());
        bytecode.append(d->committed.bytecode.constData(), d->committed.bytecode.size());

        QByteArray data = d->committed.data;
        while (data.count() % 4) data.append('\0');
        prog.signalTableOffset = data.count();
        data += d->buildSignalTable();
        while (data.count() % 4) data.append('\0');
        prog.exceptionDataOffset = data.count();
        data += d->buildExceptionData();

        prog.dataLength = 4 * ((data.size() + 3) / 4);
        prog.subscriptions = d->committed.subscriptionCount;
        prog.instructionCount = bytecode.count();
        int size = sizeof(QV4Program) + bytecode.count();
        size += prog.dataLength;

        programData.resize(size);
        memcpy(programData.data(), &prog, sizeof(QV4Program));
        if (prog.dataLength)
            memcpy((char *)((QV4Program *)programData.data())->data(), data.constData(), 
                   data.size());
        memcpy((char *)((QV4Program *)programData.data())->instructions(), bytecode.constData(),
               bytecode.count());
    } 

    if (bindingsDump()) {
        qWarning().nospace() << "Subscription slots:";

        QQmlAssociationList<QString, int> subscriptionIds;
        foreach (subscriptionIds, d->committed.subscriptions) {
            for (QQmlAssociationList<QString, int>::ConstIterator iter = subscriptionIds.begin();
                 iter != subscriptionIds.end(); ++iter) {
                qWarning().nospace() << "    " << iter->first << "\t-> " << iter->second;
            }
        }
        QV4Compiler::dump(programData);
    }

    return programData;
}

void QV4Compiler::enableBindingsTest(bool e)
{
    if (e)
        qmlBindingsTest = true;
    else 
        qmlBindingsTest = qmlBindingsTestEnv();
}

void QV4Compiler::enableV4(bool e)
{
    qmlEnableV4 = e;
}

QT_END_NAMESPACE
