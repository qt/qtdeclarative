/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMLTCCOMPILERPIECES_H
#define QMLTCCOMPILERPIECES_H

#include <QtCore/qscopeguard.h>
#include <QtCore/qstringbuilder.h>
#include <QtCore/qfileinfo.h>

#include "qmltcoutputir.h"
#include "qmltcvisitor.h"

QT_BEGIN_NAMESPACE

/*!
    \internal

    Helper class that generates different code for the output IR. Takes care of
    complicated, repetitive, nasty logic which is better kept in a single
    confined place
*/
struct QmltcCodeGenerator
{
    static const QString privateEngineName;
    QString documentUrl;
    QQmlJSScope::ConstPtr documentRoot;

    /*!
        \internal

        Generates \a current.init 's code which sets up the QQmlContext for \a
        type. Returns a QScopeGuard with the final instructions of the function
        that have to be generated at a later point, once everything else is
        compiled.
    */
    [[nodiscard]] inline decltype(auto) generate_qmlContextSetup(QmltcType &current,
                                                                 const QQmlJSScope::ConstPtr &type);

    inline void generate_assignToProperty(QmltcType &current, const QQmlJSScope::ConstPtr &type,
                                          const QQmlJSMetaProperty &p, const QString &value,
                                          const QString &accessor);

    QString urlMethodName() const
    {
        QFileInfo fi(documentUrl);
        return u"q_qmltc_docUrl_" + fi.fileName().replace(u".qml"_qs, u""_qs).replace(u'.', u'_');
    }
};

inline decltype(auto)
QmltcCodeGenerator::generate_qmlContextSetup(QmltcType &current, const QQmlJSScope::ConstPtr &type)
{
    // qmltc_init()'s parameters:
    // * QQmltcObjectCreationHelper* creator
    // * QQmlEngine* engine
    // * const QQmlRefPointer<QQmlContextData>& parentContext
    // * bool canFinalize [optional, when document root]
    const bool isDocumentRoot = type == documentRoot;

    current.init.body << u"Q_UNUSED(creator);"_qs; // can happen sometimes

    current.init.body << u"auto context = parentContext;"_qs;

    // if parent scope is a QML type and is not a (current) document root, the
    // parentContext we passed as input to this object is a context of another
    // document. we need to fix it by using parentContext->parent()
    if (auto parentScope = type->parentScope(); parentScope && parentScope->isComposite()
        && parentScope->scopeType() == QQmlJSScope::QMLScope && parentScope != documentRoot) {
        current.init.body << u"// NB: context->parent() is the context of this document"_qs;
        current.init.body << u"context = context->parent();"_qs;
    }

    // any object with QML base class has to call base's init method
    if (auto base = type->baseType(); base->isComposite()) {
        QString lhs;
        // init creates new context. for document root, it's going to be a real
        // parent context, so store it temporarily in `context` variable
        if (isDocumentRoot)
            lhs = u"context = "_qs;
        current.init.body << u"// 0. call base's init method"_qs;
        current.init.body << QStringLiteral(
                                     "%1%2::%3(creator, engine, context, /* finalize */ false)")
                                     .arg(lhs, base->internalName(), current.init.name);
    }

    current.init.body
            << QStringLiteral("auto %1 = QQmlEnginePrivate::get(engine);").arg(privateEngineName);
    current.init.body << QStringLiteral("Q_UNUSED(%1);").arg(privateEngineName); // precaution

    // when generating root, we need to create a new (document-level) context.
    // otherwise, just use existing context as is
    if (isDocumentRoot) {
        current.init.body << u"// 1. create new QML context for this document"_qs;
        // TODO: the last 2 parameters are {0, true} because we deal with
        // document root only here. in reality, there are inline components
        // which need { index, true } instead
        current.init.body
                << QStringLiteral(
                           "context = %1->createInternalContext(%1->compilationUnitFromUrl(%2()), "
                           "context, 0, true);")
                           .arg(privateEngineName, urlMethodName());
    } else {
        current.init.body << u"// 1. use current context as this object's context"_qs;
        current.init.body << u"// context = context;"_qs;
    }

    if (!type->baseType()->isComposite() || isDocumentRoot) {
        current.init.body << u"// 2. set context for this object"_qs;
        current.init.body << QStringLiteral(
                                     "%1->setInternalContext(this, context, QQmlContextData::%2);")
                                     .arg(privateEngineName,
                                          (isDocumentRoot ? u"DocumentRoot"_qs
                                                          : u"OrdinaryObject"_qs));
        if (isDocumentRoot)
            current.init.body << u"context->setContextObject(this);"_qs;
    }

    if (int id = type->runtimeId(); id >= 0) {
        current.init.body << u"// 3. set id since it is provided"_qs;
        current.init.body << u"context->setIdValue(%1, this);"_qs.arg(QString::number(id));
    }

    // TODO: add QQmlParserStatus::classBegin() to init

    const auto generateFinalLines = [&current, isDocumentRoot]() {
        if (isDocumentRoot) {
            current.init.body << u"// 4. call finalize in the document root"_qs;
            current.init.body << u"if (canFinalize) {"_qs;
            current.init.body << QStringLiteral("    %1(creator, engine, /* finalize */ true);")
                                         .arg(current.finalize.name);
            current.init.body << u"}"_qs;
        }
        current.init.body << u"return context;"_qs;
    };

    return QScopeGuard(generateFinalLines);
}

inline void QmltcCodeGenerator::generate_assignToProperty(QmltcType &current,
                                                          const QQmlJSScope::ConstPtr &type,
                                                          const QQmlJSMetaProperty &p,
                                                          const QString &value,
                                                          const QString &accessor)
{
    Q_ASSERT(p.isValid());
    Q_ASSERT(!p.isList()); // TODO: check this one
    Q_ASSERT(!p.isPrivate());
    Q_UNUSED(type);

    QStringList code;
    code.reserve(6); // always should be enough

    if (!p.isWritable()) {
        Q_ASSERT(type->hasOwnProperty(p.propertyName())); // otherwise we cannot set it
        code << u"%1->m_%2 = %3;"_qs.arg(accessor, p.propertyName(), value);
    } else {
        const QString setter = p.write();
        Q_ASSERT(!setter.isEmpty()); // in practice could be empty, but ignore it for now
        code << u"%1->%2(%3);"_qs.arg(accessor, setter, value);
    }

    current.init.body += code;
}

QT_END_NAMESPACE

#endif // QMLTCCOMPILERPIECES_H
