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

#include "qv4irbuilder_p.h"
#include "qv4compiler_p_p.h"

#include <private/qqmlglobal_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmltypenamecache_p.h>

DEFINE_BOOL_CONFIG_OPTION(qmlVerboseCompiler, QML_VERBOSE_COMPILER)

QT_BEGIN_NAMESPACE

using namespace QQmlJS;

static IR::Type irTypeFromVariantType(int t, QQmlEnginePrivate *engine)
{
    switch (t) {
    case QMetaType::Bool:
        return IR::BoolType;

    case QMetaType::Int:
        return IR::IntType;

    case QMetaType::Float:
        return IR::FloatType;

    case QMetaType::Double:
        return IR::NumberType;

    case QMetaType::QString:
        return IR::StringType;

    case QMetaType::QUrl:
        return IR::UrlType;

    case QMetaType::QColor:
        return IR::ColorType;

    default:
        if (t == QQmlMetaType::QQuickAnchorLineMetaTypeId()) {
            return IR::SGAnchorLineType;
        } else if (!engine->metaObjectForType(t).isNull()) {
            return IR::ObjectType;
        } else if (t == qMetaTypeId<QJSValue>()) {
            return IR::JSValueType;
        }

        return IR::InvalidType;
    }
}

QV4IRBuilder::QV4IRBuilder(const QV4Compiler::Expression *expr,
                           QQmlEnginePrivate *engine)
: m_expression(expr), m_engine(engine), _function(0), _block(0), _discard(false),
  _invalidatable(false)
{
}

bool QV4IRBuilder::operator()(QQmlJS::IR::Function *function,
                              QQmlJS::AST::Node *ast, bool *invalidatable)
{
    bool discarded = false;

    IR::BasicBlock *block = function->newBasicBlock();

    qSwap(_discard, discarded);
    qSwap(_function, function);
    qSwap(_block, block);

    ExprResult r;
    AST::SourceLocation location;
    if (AST::ExpressionNode *asExpr = ast->expressionCast()) {
        r = expression(asExpr);
        location = asExpr->firstSourceLocation();
    } else if (AST::Statement *asStmt = ast->statementCast()) {
        r = statement(asStmt);
        location = asStmt->firstSourceLocation();
    }

    //_block->MOVE(_block->TEMP(IR::InvalidType), r.code);
    if (r.code) {
        IR::Type targetType = IR::InvalidType;

        // This is the only operation where variant is supported:
        QQmlPropertyData *data = &m_expression->property->core;
        if (data->propType == QMetaType::QVariant) {
            targetType = (data->isVarProperty() ? IR::VarType : IR::VariantType);
        } else {
            targetType = irTypeFromVariantType(data->propType, m_engine);
        }

        if (targetType != r.type()) {
            IR::Expr *x = _block->TEMP(targetType);
            _block->MOVE(x, r, true);
            r.code = x;
        }
        _block->RET(r.code, targetType, location.startLine, location.startColumn);
    }

    qSwap(_block, block);
    qSwap(_function, function);
    qSwap(_discard, discarded);

    *invalidatable = _invalidatable;
    return !discarded;
}

bool QV4IRBuilder::buildName(QList<QStringRef> &name,
                                              AST::Node *node,
                                              QList<AST::ExpressionNode *> *nodes)
{
    if (node->kind == AST::Node::Kind_IdentifierExpression) {
        name << static_cast<AST::IdentifierExpression*>(node)->name;
        if (nodes) *nodes << static_cast<AST::IdentifierExpression*>(node);
    } else if (node->kind == AST::Node::Kind_FieldMemberExpression) {
        AST::FieldMemberExpression *expr =
            static_cast<AST::FieldMemberExpression *>(node);

        if (!buildName(name, expr->base, nodes))
            return false;

        name << expr->name;
        if (nodes) *nodes << expr;
    } else {
        return false;
    }

    return true;
}

void QV4IRBuilder::discard() 
{ 
    _discard = true; 
}

QV4IRBuilder::ExprResult 
QV4IRBuilder::expression(AST::ExpressionNode *ast)
{
    ExprResult r;
    if (ast) {
        qSwap(_expr, r);
        accept(ast);
        qSwap(_expr, r);

        if (r.is(IR::InvalidType))
            discard();
        else {
            Q_ASSERT(r.hint == r.format);
        }
    }

    return r;
}

void QV4IRBuilder::condition(AST::ExpressionNode *ast, IR::BasicBlock *iftrue, IR::BasicBlock *iffalse)
{
    if (! ast)
        return;
    ExprResult r(iftrue, iffalse);
    qSwap(_expr, r);
    accept(ast);
    qSwap(_expr, r);

    if (r.format != ExprResult::cx) {
        if (! r.code)
            discard();

        Q_ASSERT(r.hint == ExprResult::cx);
        Q_ASSERT(r.format == ExprResult::ex);

        if (r.type() != IR::BoolType) {
            IR::Temp *t = _block->TEMP(IR::BoolType);
            _block->MOVE(t, r);
            r = t;
        }

        _block->CJUMP(_block->UNOP(IR::OpIfTrue, r), iftrue, iffalse);
    }
}

QV4IRBuilder::ExprResult
QV4IRBuilder::statement(AST::Statement *ast)
{
    ExprResult r;
    if (ast) {
        qSwap(_expr, r);
        accept(ast);
        qSwap(_expr, r);

        if (r.is(IR::InvalidType))
            discard();
        else {
            Q_ASSERT(r.hint == r.format);
        }
    }

    return r;
}

void QV4IRBuilder::sourceElement(AST::SourceElement *ast)
{
    accept(ast);
}

void QV4IRBuilder::implicitCvt(ExprResult &expr, IR::Type type)
{
    if (expr.type() == type)
        return; // nothing to do

    IR::Expr *x = _block->TEMP(type);
    _block->MOVE(x, expr.code);
    expr.code = x;
}

// QML
bool QV4IRBuilder::visit(AST::UiProgram *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::UiImportList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::UiImport *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::UiPublicMember *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::UiSourceElement *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::UiObjectDefinition *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::UiObjectInitializer *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::UiObjectBinding *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::UiScriptBinding *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::UiArrayBinding *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::UiObjectMemberList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::UiArrayMemberList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::UiQualifiedId *)
{
    Q_ASSERT(!"unreachable");
    return false;
}


// JS
bool QV4IRBuilder::visit(AST::Program *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::SourceElements *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::FunctionSourceElement *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::StatementSourceElement *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

// object literals
bool QV4IRBuilder::visit(AST::PropertyAssignmentList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::PropertyNameAndValue *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::PropertyGetterSetter *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::IdentifierPropertyName *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::StringLiteralPropertyName *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::NumericLiteralPropertyName *)
{
    Q_ASSERT(!"unreachable");
    return false;
}


// array literals
bool QV4IRBuilder::visit(AST::ElementList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool QV4IRBuilder::visit(AST::Elision *)
{
    Q_ASSERT(!"unreachable");
    return false;
}


// function calls
bool QV4IRBuilder::visit(AST::ArgumentList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

// expressions
bool QV4IRBuilder::visit(AST::ObjectLiteral *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::ArrayLiteral *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::ThisExpression *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::IdentifierExpression *ast)
{
    const quint16 line = ast->identifierToken.startLine;
    const quint16 column = ast->identifierToken.startColumn;

    const QString name = ast->name.toString();

    if (name.at(0) == QLatin1Char('u') && name.length() == 9 && name == QLatin1String("undefined")) {
        _expr.code = _block->CONST(IR::UndefinedType, 0); // ### undefined value
    } else if (m_engine->v8engine()->illegalNames().contains(name) ) {
        if (qmlVerboseCompiler()) qWarning() << "*** illegal symbol:" << name;
        return false;
    } else if (const QQmlScript::Object *obj = m_expression->ids->value(name)) {
        IR::Name *code = _block->ID_OBJECT(name, obj, line, column);
        if (obj == m_expression->component)
            code->storage = IR::Name::RootStorage;
        _expr.code = code;
    } else {

        QQmlTypeNameCache::Result r = m_expression->importCache->query(name);
        if (r.isValid()) {
            if (r.type) {
                if (r.type->isSingleton()) {
                    // Note: we don't need to check singletonType->qobjectCallback here, since
                    // we did that check in registerSingletonType() in qqmlmetatype.cpp.
                    // We cannot create the QObject Singleton Type Instance here,
                    // as we might be running in a loader thread.
                    // Thus, V4 can only handle bindings which use Singleton Types which
                    // were registered with the templated registration function.
                    _expr.code = _block->SINGLETON_OBJECT(name, r.type->singletonInstanceInfo()->instanceMetaObject, IR::Name::MemberStorage, line, column);
                } else {
                    _expr.code = _block->ATTACH_TYPE(name, r.type, IR::Name::ScopeStorage, line, column);
                }
            }
        } else {
            bool found = false;

            if (m_expression->context != m_expression->component) {
                // RootStorage is more efficient than ScopeStorage, so prefer that if they are the same
                QQmlPropertyCache *cache = m_expression->context->synthCache;
                if (!cache) cache = m_expression->context->metatype;

                QQmlPropertyData *data = cache->property(name, 0, 0);

                if (data && data->hasRevision()) {
                    if (qmlVerboseCompiler()) 
                        qWarning() << "*** versioned symbol:" << name;
                    discard();
                    return false;
                }

                if (data && !data->isFunction()) {
                    IR::Type irType = irTypeFromVariantType(data->propType, m_engine);
                    _expr.code = _block->SYMBOL(irType, name, QQmlMetaObject(cache), data, IR::Name::ScopeStorage, line, column);
                    found = true;
                } 
            }

            if (!found) {
                QQmlPropertyCache *cache = m_expression->component->synthCache;
                if (!cache) cache = m_expression->component->metatype;

                QQmlPropertyData *data = cache->property(name, 0, 0);

                if (data && data->hasRevision()) {
                    if (qmlVerboseCompiler()) 
                        qWarning() << "*** versioned symbol:" << name;
                    discard();
                    return false;
                }

                if (data && !data->isFunction()) {
                    IR::Type irType = irTypeFromVariantType(data->propType, m_engine);
                    _expr.code = _block->SYMBOL(irType, name, QQmlMetaObject(cache), data, IR::Name::RootStorage, line, column);
                    found = true;
                } 
            }

            if (!found && qmlVerboseCompiler())
                qWarning() << "*** unknown symbol:" << name;
        }
    }

    if (_expr.code && _expr.hint == ExprResult::cx) {
        _expr.format = ExprResult::cx;

        if (_expr.type() != IR::BoolType) {
            IR::Temp *t = _block->TEMP(IR::BoolType);
            _block->MOVE(t, _expr);
            _expr.code = t;
        }

        _block->CJUMP(_expr.code, _expr.iftrue, _expr.iffalse);
        _expr.code = 0;
    }

    return false;
}

bool QV4IRBuilder::visit(AST::NullExpression *)
{
    // ### TODO: cx format
    _expr.code = _block->CONST(IR::NullType, 0);
    return false;
}

bool QV4IRBuilder::visit(AST::TrueLiteral *)
{
    // ### TODO: cx format
    _expr.code = _block->CONST(IR::BoolType, 1);
    return false;
}

bool QV4IRBuilder::visit(AST::FalseLiteral *)
{
    // ### TODO: cx format
    _expr.code = _block->CONST(IR::BoolType, 0);
    return false;
}

bool QV4IRBuilder::visit(AST::StringLiteral *ast)
{
    // ### TODO: cx format
    _expr.code = _block->STRING(ast->value);
    return false;
}

bool QV4IRBuilder::visit(AST::NumericLiteral *ast)
{
    if (_expr.hint == ExprResult::cx) {
        _expr.format = ExprResult::cx;
        _block->JUMP(ast->value ? _expr.iftrue : _expr.iffalse);
    } else {
        _expr.code = _block->CONST(IR::NumberType, ast->value);
    }
    return false;
}

bool QV4IRBuilder::visit(AST::RegExpLiteral *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::NestedExpression *)
{
    return true; // the value of the nested expression
}

bool QV4IRBuilder::visit(AST::ArrayMemberExpression *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::FieldMemberExpression *ast)
{
    if (IR::Expr *left = expression(ast->base)) {
        if (IR::Name *baseName = left->asName()) {
            const quint32 line = ast->identifierToken.startLine;
            const quint32 column = ast->identifierToken.startColumn;

            QString name = ast->name.toString();

            switch(baseName->symbol) {
            case IR::Name::Unbound:
                break;

            case IR::Name::AttachType:
                if (name.at(0).isUpper()) {
                    QByteArray utf8Name = name.toUtf8();
                    const char *enumName = utf8Name.constData();

                    const QMetaObject *meta = baseName->declarativeType->metaObject();
                    bool found = false;
                    for (int ii = 0; !found && ii < meta->enumeratorCount(); ++ii) {
                        QMetaEnum e = meta->enumerator(ii);
                        for (int jj = 0; !found && jj < e.keyCount(); ++jj) {
                            if (0 == strcmp(e.key(jj), enumName)) {
                                found = true;
                                _expr.code = _block->CONST(IR::IntType, e.value(jj));
                            }
                        }
                    }

                    if (!found && qmlVerboseCompiler())
                        qWarning() << "*** unresolved enum:" 
                                   << (*baseName->id + QLatin1Char('.') + ast->name.toString());
                } else if(const QMetaObject *attachedMeta = baseName->declarativeType->attachedPropertiesType()) {
                    QQmlPropertyCache *cache = m_engine->cache(attachedMeta);
                    QQmlPropertyData *data = cache->property(name, 0, 0);

                    if (!data || data->isFunction())
                        return false; // Don't support methods (or non-existing properties ;)

                    if (!data->isFinal())
                        _invalidatable = true;

                    IR::Type irType = irTypeFromVariantType(data->propType, m_engine);
                    _expr.code = _block->SYMBOL(baseName, irType, name, attachedMeta, data, line, column);
                }
                break;

            case IR::Name::SingletonObject: {
                if (name.at(0).isUpper()) {
                    QByteArray utf8Name = name.toUtf8();
                    const char *enumName = utf8Name.constData();

                    const QQmlPropertyCache *cache = baseName->meta.propertyCache(m_engine);
                    if (!cache) {
                        //Happens in some cases where they make properties with uppercase names
                        qFatal("QV4: Unable to resolve enum: '%s'",
                               QString(*baseName->id + QLatin1Char('.') + ast->name.toString()).toLatin1().constData());
                    }

                    const QMetaObject *meta = cache->firstCppMetaObject();
                    bool found = false;
                    for (int ii = 0; !found && ii < meta->enumeratorCount(); ++ii) {
                        QMetaEnum e = meta->enumerator(ii);
                        for (int jj = 0; !found && jj < e.keyCount(); ++jj) {
                            if (0 == strcmp(e.key(jj), enumName)) {
                                found = true;
                                _expr.code = _block->CONST(IR::IntType, e.value(jj));
                            }
                        }
                    }
                    if (!found && qmlVerboseCompiler())
                        qWarning() << "*** unresolved enum:"
                                   << (*baseName->id + QLatin1Char('.') + ast->name.toString());
                } else {
                    QQmlPropertyCache *cache = baseName->meta.propertyCache(m_engine);
                    if (!cache) return false;
                    QQmlPropertyData *data = cache->property(name, 0, 0);

                    if (!data || data->isFunction())
                        return false; // Don't support methods (or non-existing properties ;)

                    if (!data->isFinal())
                        _invalidatable = true;

                    IR::Type irType = irTypeFromVariantType(data->propType, m_engine);
                    _expr.code = _block->SYMBOL(baseName, irType, name, baseName->meta, data, line, column);
                }
            }
            break;

            case IR::Name::IdObject: {
                const QQmlScript::Object *idObject = baseName->idObject;
                QQmlPropertyCache *cache = 
                    idObject->synthCache?idObject->synthCache:idObject->metatype;

                QQmlPropertyData *data = cache->property(name, 0, 0);

                if (!data || data->isFunction())
                    return false; // Don't support methods (or non-existing properties ;)

                if (data->hasRevision()) {
                    if (qmlVerboseCompiler()) 
                        qWarning() << "*** versioned symbol:" << name;
                    discard();
                    return false;
                }

                IR::Type irType = irTypeFromVariantType(data->propType, m_engine);
                _expr.code = _block->SYMBOL(baseName, irType, name, QQmlMetaObject(cache), data, line, column);
                }
                break;

            case IR::Name::Property: 
                if (baseName->type == IR::ObjectType && !baseName->meta.isNull()) {
                    QQmlMetaObject meta = m_engine->metaObjectForType(baseName->property->propType);
                    QQmlPropertyCache *cache = meta.propertyCache(m_engine);
                    if (!cache)
                        return false;

                    if (QQmlPropertyData *data = cache->property(name, 0, 0)) {
                        if (!baseName->property->isFinal() || !data->isFinal())
                            _invalidatable = true;

                        IR::Type irType = irTypeFromVariantType(data->propType, m_engine);
                        _expr.code = _block->SYMBOL(baseName, irType, name,
                                                    meta, data, line, column);
                    }
                }
                break;

            case IR::Name::Object: 
            case IR::Name::Slot:
                break;
            }
        }
    }

    return false;
}

bool QV4IRBuilder::preVisit(AST::Node *)
{
    return ! _discard;
}

bool QV4IRBuilder::visit(AST::NewMemberExpression *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::NewExpression *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::CallExpression *ast)
{
    QList<QStringRef> names;
    QList<AST::ExpressionNode *> nameNodes;

    names.reserve(4);
    nameNodes.reserve(4);

    if (buildName(names, ast->base, &nameNodes)) {
        //ExprResult base = expression(ast->base);
        QString id;
        for (int i = 0; i < names.size(); ++i) {
            if (i)
                id += QLatin1Char('.');
            id += names.at(i);
        }
        const AST::SourceLocation loc = nameNodes.last()->firstSourceLocation();
        IR::Expr *base = _block->NAME(id, loc.startLine, loc.startColumn);

        IR::ExprList *args = 0, **argsInserter = &args;
        for (AST::ArgumentList *it = ast->arguments; it; it = it->next) {
            IR::Expr *arg = expression(it->expression);
            *argsInserter = _function->pool->New<IR::ExprList>();
            (*argsInserter)->init(arg);
            argsInserter = &(*argsInserter)->next;
        }

        IR::Temp *r = _block->TEMP(IR::InvalidType);
        IR::Expr *call = _block->CALL(base, args);
        _block->MOVE(r, call);
        r->type = call->type;
        _expr.code = r;
    }

    return false;
}

bool QV4IRBuilder::visit(AST::PostIncrementExpression *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::PostDecrementExpression *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::DeleteExpression *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::VoidExpression *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::TypeOfExpression *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::PreIncrementExpression *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::PreDecrementExpression *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::UnaryPlusExpression *ast)
{
    ExprResult expr = expression(ast->expression);
    if (expr.isNot(IR::InvalidType)) {
        if (expr.code->asConst() != 0) {
            _expr = expr;
            return false;
        }

        IR::Expr *code = _block->UNOP(IR::OpUPlus, expr);
        _expr.code = _block->TEMP(code->type);
        _block->MOVE(_expr, code);
    }

    return false;
}

bool QV4IRBuilder::visit(AST::UnaryMinusExpression *ast)
{
    ExprResult expr = expression(ast->expression);
    if (expr.isNot(IR::InvalidType)) {
        if (IR::Const *c = expr.code->asConst()) {
            _expr = expr;
            _expr.code = _block->CONST(expr->type, -c->value);
            return false;
        }

        IR::Expr *code = _block->UNOP(IR::OpUMinus, expr);
        _expr.code = _block->TEMP(code->type);
        _block->MOVE(_expr, code);
    }

    return false;
}

bool QV4IRBuilder::visit(AST::TildeExpression *ast)
{
    ExprResult expr = expression(ast->expression);
    if (expr.isNot(IR::InvalidType)) {
        if (IR::Const *c = expr.code->asConst()) {
            _expr = expr;
            _expr.code = _block->CONST(expr->type, ~int(c->value));
            return false;
        }
        IR::Expr *code = _block->UNOP(IR::OpCompl, expr);
        _expr.code = _block->TEMP(code->type);
        _block->MOVE(_expr, code);
    }

    return false;
}

bool QV4IRBuilder::visit(AST::NotExpression *ast)
{
    ExprResult expr = expression(ast->expression);

    if (expr.isNot(IR::InvalidType)) {
        if (IR::Const *c = expr.code->asConst()) {
            _expr = expr;
            _expr.code = _block->CONST(IR::BoolType, !c->value);
            return false;
        }

        IR::Expr *code = _block->UNOP(IR::OpNot, expr);
        _expr.code = _block->TEMP(code->type);
        _block->MOVE(_expr, code);

    } else if (expr.hint == ExprResult::cx) {
        expr.format = ExprResult::cx;
        _block->CJUMP(_block->UNOP(IR::OpNot, expr), _expr.iftrue, _expr.iffalse);
        return false;
    }

    return false;
}

void QV4IRBuilder::binop(AST::BinaryExpression *ast, ExprResult left, ExprResult right)
{
    if (IR::Type t = maxType(left.type(), right.type())) {
        if (!left->asConst() && !right->asConst()) {
            // the implicit conversions are needed only
            // when compiling non-constant expressions.
            implicitCvt(left, t);
            implicitCvt(right, t);
        }
    } else if ((left.type() != IR::ObjectType && left.type() != IR::NullType) ||
               (right.type() != IR::ObjectType && right.type() != IR::NullType))
        return;

    if (_expr.hint == ExprResult::cx) {
        _expr.format = ExprResult::cx;
        _block->CJUMP(_block->BINOP(IR::binaryOperator(ast->op), left, right), _expr.iftrue, _expr.iffalse);
    } else {
        IR::Expr *e = _block->BINOP(IR::binaryOperator(ast->op), left, right);
        if (e->asConst() != 0 || e->asString() != 0)
            _expr.code = e;
        else {
            IR::Temp *t = _block->TEMP(e->type);
            _block->MOVE(t, e);
            _expr.code = t;
        }
    }
}

bool QV4IRBuilder::visit(AST::BinaryExpression *ast)
{
    switch (ast->op) {
    case QSOperator::And: {
        if (_expr.hint == ExprResult::cx) {
            _expr.format = ExprResult::cx;

            Q_ASSERT(_expr.iffalse != 0);
            Q_ASSERT(_expr.iftrue != 0);

            IR::BasicBlock *iftrue = _function->newBasicBlock();
            condition(ast->left, iftrue, _expr.iffalse);

            _block = iftrue;
            condition(ast->right, _expr.iftrue, _expr.iffalse);
        } else {
            IR::BasicBlock *iftrue = _function->newBasicBlock();
            IR::BasicBlock *iffalse = _function->newBasicBlock();
            IR::BasicBlock *endif = _function->newBasicBlock();

            ExprResult left = expression(ast->left);
            IR::Temp *cond = _block->TEMP(IR::BoolType);
            _block->MOVE(cond, left);
            _block->CJUMP(cond, iftrue, iffalse);

            IR::Temp *r = _block->TEMP(IR::InvalidType);

            _block = iffalse;
            _block->MOVE(r, cond);
            _block->JUMP(endif);

            _block = iftrue;
            ExprResult right = expression(ast->right);
            _block->MOVE(r, right);
            _block->JUMP(endif);

            if (left.type() != right.type())
                discard();

            _block = endif;

            r->type = right.type();
            _expr.code = r;
        }
    } break;

    case QSOperator::Or: {
        IR::BasicBlock *iftrue = _function->newBasicBlock();
        IR::BasicBlock *endif = _function->newBasicBlock();

        ExprResult left = expression(ast->left);
        IR::Temp *r = _block->TEMP(left.type());
        _block->MOVE(r, left);

        IR::Expr *cond = r;
        if (r->type != IR::BoolType) {
            cond = _block->TEMP(IR::BoolType);
            _block->MOVE(cond, r);
        }

        _block->CJUMP(_block->UNOP(IR::OpNot, cond), iftrue, endif);

        _block = iftrue;
        ExprResult right = expression(ast->right);
        _block->MOVE(r, right);
        _block->JUMP(endif);

        if (left.type() != right.type())
            discard();

        _expr.code = r;

        _block = endif;
    } break;

    case QSOperator::Lt:
    case QSOperator::Gt:
    case QSOperator::Le:
    case QSOperator::Ge: {
        ExprResult left = expression(ast->left);
        ExprResult right = expression(ast->right);
        if (left.type() == IR::StringType && right.type() == IR::StringType) {
            binop(ast, left, right);
        } else if (left.isValid() && right.isValid()) {
            implicitCvt(left, IR::NumberType);
            implicitCvt(right, IR::NumberType);
            binop(ast, left, right);
        }
    } break;

    case QSOperator::NotEqual:
    case QSOperator::Equal: {
        ExprResult left = expression(ast->left);
        ExprResult right = expression(ast->right);
        if ((left.type() == IR::NullType || left.type() == IR::UndefinedType) &&
                (right.type() == IR::NullType || right.type() == IR::UndefinedType)) {
            const bool isEq = ast->op == QSOperator::Equal;
            if (_expr.hint == ExprResult::cx) {
                _expr.format = ExprResult::cx;
                _block->JUMP(isEq ? _expr.iftrue : _expr.iffalse);
            } else {
                _expr.code = _block->CONST(IR::BoolType, isEq ? 1 : 0);
            }
        } else if ((left.type() == IR::StringType && right.type() >= IR::FirstNumberType) ||
                   (left.type() >= IR::FirstNumberType && right.type() == IR::StringType)) {
            implicitCvt(left, IR::NumberType);
            implicitCvt(right, IR::NumberType);
            binop(ast, left, right);
        } else if (left.isValid() && right.isValid()) {
            binop(ast, left, right);
        }
    } break;

    case QSOperator::StrictEqual:
    case QSOperator::StrictNotEqual: {
        ExprResult left = expression(ast->left);
        ExprResult right = expression(ast->right);
        if (left.type() == right.type()) {
            binop(ast, left, right);
        } else if (left.type() > IR::BoolType && right.type() > IR::BoolType) {
            // left and right have numeric type (int or real)
            binop(ast, left, right);
        } else if ((left.type() == IR::ObjectType && right.type() == IR::NullType) ||
                   (right.type() == IR::ObjectType && left.type() == IR::NullType)) {
            // comparing a qobject with null
            binop(ast, left, right);
        } else if (left.isValid() && right.isValid()) {
            // left and right have different types
            const bool isEq = ast->op == QSOperator::StrictEqual;
            if (_expr.hint == ExprResult::cx) {
                _expr.format = ExprResult::cx;
                _block->JUMP(isEq ? _expr.iffalse : _expr.iftrue);
            } else {
                _expr.code = _block->CONST(IR::BoolType, isEq ? 0 : 1);
            }
        }
    } break;

    case QSOperator::BitAnd:
    case QSOperator::BitOr:
    case QSOperator::BitXor:
    case QSOperator::LShift:
    case QSOperator::RShift:
    case QSOperator::URShift: {
        ExprResult left = expression(ast->left);
        if (left.is(IR::InvalidType))
            return false;

        ExprResult right = expression(ast->right);
        if (right.is(IR::InvalidType))
            return false;

        implicitCvt(left, IR::IntType);
        implicitCvt(right, IR::IntType);

        IR::Expr *code = _block->BINOP(IR::binaryOperator(ast->op), left, right);
        _expr.code = _block->TEMP(code->type);
        _block->MOVE(_expr.code, code);

    } break;

    case QSOperator::Add: {
        ExprResult left = expression(ast->left);
        if (left.is(IR::InvalidType))
            return false;

        ExprResult right = expression(ast->right);
        if (right.is(IR::InvalidType))
            return false;

        if (left.isPrimitive() && right.isPrimitive()) {
            if (left.type() == IR::StringType || right.type() == IR::StringType) {
                implicitCvt(left, IR::StringType);
                implicitCvt(right, IR::StringType);
            }
            binop(ast, left, right);
        }
    } break;

    case QSOperator::Div:
    case QSOperator::Mod:
    case QSOperator::Mul:
    case QSOperator::Sub: {
        ExprResult left = expression(ast->left);
        if (left.is(IR::InvalidType))
            return false;

        ExprResult right = expression(ast->right);
        if (right.is(IR::InvalidType))
            return false;

        IR::Type t = maxType(left.type(), right.type());
        if (t >= IR::FirstNumberType) {
            implicitCvt(left, IR::NumberType);
            implicitCvt(right, IR::NumberType);

            IR::Expr *code = _block->BINOP(IR::binaryOperator(ast->op), left, right);
            _expr.code = _block->TEMP(code->type);
            _block->MOVE(_expr.code, code);
        }
    } break;

    case QSOperator::In:
    case QSOperator::InstanceOf:
    case QSOperator::Assign:
    case QSOperator::InplaceAnd:
    case QSOperator::InplaceSub:
    case QSOperator::InplaceDiv:
    case QSOperator::InplaceAdd:
    case QSOperator::InplaceLeftShift:
    case QSOperator::InplaceMod:
    case QSOperator::InplaceMul:
    case QSOperator::InplaceOr:
    case QSOperator::InplaceRightShift:
    case QSOperator::InplaceURightShift:
    case QSOperator::InplaceXor:
        // yup, we don't do those.
        break;
    } // switch

    return false;
}

bool QV4IRBuilder::visit(AST::ConditionalExpression *ast)
{
    IR::BasicBlock *iftrue = _function->newBasicBlock();
    IR::BasicBlock *iffalse = _function->newBasicBlock();
    IR::BasicBlock *endif = _function->newBasicBlock();

    condition(ast->expression, iftrue, iffalse);

    IR::Temp *r = _block->TEMP(IR::InvalidType);

    qSwap(_block, iftrue);
    ExprResult ok = expression(ast->ok);
    _block->MOVE(r, ok);
    _block->JUMP(endif);
    qSwap(_block, iftrue);

    qSwap(_block, iffalse);
    ExprResult ko = expression(ast->ko);
    _block->MOVE(r, ko);
    _block->JUMP(endif);
    qSwap(_block, iffalse);

    r->type = maxType(ok.type(), ko.type());
    _expr.code = r;

    _block = endif;

    return false;
}

bool QV4IRBuilder::visit(AST::Expression *ast)
{
    _block->EXP(expression(ast->left));
    _expr = expression(ast->right);

    return false;
}


// statements
bool QV4IRBuilder::visit(AST::Block *ast)
{
    if (ast->statements && ! ast->statements->next) {
        // we have one and only one statement
        accept(ast->statements->statement);
    }

    return false;
}

bool QV4IRBuilder::visit(AST::StatementList *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::VariableStatement *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::VariableDeclarationList *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::VariableDeclaration *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::EmptyStatement *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::ExpressionStatement *ast)
{
    if (ast->expression) {
         // return the value of this expression
        return true;
    }

    return false;
}

bool QV4IRBuilder::visit(AST::IfStatement *ast)
{
    if (! ast->ko) {
        // This is an if statement without an else branch.
        discard();
    } else {
        IR::BasicBlock *iftrue = _function->newBasicBlock();
        IR::BasicBlock *iffalse = _function->newBasicBlock();
        IR::BasicBlock *endif = _function->newBasicBlock();

        condition(ast->expression, iftrue, iffalse);

        IR::Temp *r = _block->TEMP(IR::InvalidType);

        qSwap(_block, iftrue);
        ExprResult ok = statement(ast->ok);
        _block->MOVE(r, ok);
        _block->JUMP(endif);
        qSwap(_block, iftrue);

        qSwap(_block, iffalse);
        ExprResult ko = statement(ast->ko);
        _block->MOVE(r, ko);
        _block->JUMP(endif);
        qSwap(_block, iffalse);

        r->type = maxType(ok.type(), ko.type());
        _expr.code = r;

        _block = endif;
    }

    return false;
}

bool QV4IRBuilder::visit(AST::DoWhileStatement *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::WhileStatement *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::ForStatement *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::LocalForStatement *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::ForEachStatement *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::LocalForEachStatement *)
{
    discard();
    return false;
}

bool QV4IRBuilder::visit(AST::ContinueStatement *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::BreakStatement *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::ReturnStatement *ast)
{
    if (ast->expression) {
        // return the value of the expression
        return true;
    }

    return false;
}

bool QV4IRBuilder::visit(AST::WithStatement *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::SwitchStatement *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::CaseBlock *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::CaseClauses *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::CaseClause *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::DefaultClause *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::LabelledStatement *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::ThrowStatement *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::TryStatement *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::Catch *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::Finally *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::FunctionDeclaration *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::FunctionExpression *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::FormalParameterList *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::FunctionBody *)
{
    return false;
}

bool QV4IRBuilder::visit(AST::DebuggerStatement *)
{
    return false;
}

QT_END_NAMESPACE
