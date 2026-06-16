// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmllscodeaction_p.h"

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lsCodeActionSupport, "qt.languageserver.codeactionsupport")

struct Tr
{
    Q_DECLARE_TR_FUNCTIONS(QQmlCodeActions)
};

using namespace Qt::StringLiterals;
using namespace QLspSpecification;
using namespace QQmlJS::Dom;

using CodeActions = QList<std::variant<Command, CodeAction>>;
using TextEdits = decltype(std::declval<TextDocumentEdit>().edits);

static QList<std::pair<QString, QQmlJS::SourceLocation>>
collectNestedIds(const QQmlLSUtils::ItemLocation &item)
{
    const auto filePtr =
            item.domItem.goToFile(item.domItem.canonicalFilePath()).ownerAs<QQmlJS::Dom::QmlFile>();
    const QString code = filePtr ? filePtr->code() : QString();

    QList<std::pair<QString, QQmlJS::SourceLocation>> innerIds;
    FileLocations::visitTree(
            item.fileLocation,
            [&code, &innerIds](const auto &, const FileLocations::Tree &t) -> bool {
                const auto idNameLoc = t->info().regions[FileLocationRegion::IdNameRegion];
                if (idNameLoc.isValid()) {
                    innerIds.append({ code.mid(idNameLoc.begin(), idNameLoc.length), idNameLoc });
                }
                return true;
            });
    return innerIds;
}

static CodeActions quickfixes(const QList<Diagnostic> &diagnostics)
{
    CodeActions codeActions;

    for (const Diagnostic &diagnostic : diagnostics) {
        if (!diagnostic.data.has_value())
            continue;

        const auto &data = diagnostic.data.value();

        int version = data[u"version"].toInt();
        QJsonArray suggestions = data[u"suggestions"].toArray();

        QList<WorkspaceEdit::DocumentChange> edits;
        QString message;
        for (const QJsonValue &suggestion : std::as_const(suggestions)) {
            message += suggestion[u"message"_s].toString() + u'\n';
            const auto &documentEdits = suggestion[u"documentEdits"_s].toArray();
            TextDocumentEdit textDocEdit;
            for (const auto &documentEdit : documentEdits) {
                TextEdit textEdit;
                textEdit.range = {
                    Position{
                            static_cast<unsigned>(documentEdit[u"lspBeginLine"].toDouble()),
                            static_cast<unsigned>(documentEdit[u"lspBeginCharacter"].toDouble()) },
                    Position{ static_cast<unsigned>(documentEdit[u"lspEndLine"].toDouble()),
                              static_cast<unsigned>(documentEdit[u"lspEndCharacter"].toDouble()) }
                };

                textEdit.newText = documentEdit[u"replacement"_s].toString().toUtf8();
                QString filename = documentEdit[u"filename"_s].toString();
                textDocEdit.textDocument = { { filename.toUtf8() }, version };
                textDocEdit.edits.append(textEdit);
            }
            edits.append(textDocEdit);
        }
        message.chop(1);
        WorkspaceEdit edit;
        edit.documentChanges = edits;

        CodeAction action;
        // VS Code and QtC ignore everything that is not a 'quickfix'.
        action.kind = CodeActionKind::QuickFix;
        action.edit = edit;
        action.title = message.toUtf8();

        codeActions.append(action);
    }
    return codeActions;
}

static TextEdit todoComment(const Position &pos, const QString &loaderId, const QString &maybeId,
                            const QList<std::pair<QString, QQmlJS::SourceLocation>> &nestedIds)
{
    const QString todo = Tr::tr("TODO:");
    const QString prefix =  "//"_L1 + QString(2 + todo.size(), u' ');

    QString comment = "// "_L1 + todo + u' '
            + Tr::tr("Move position bindings from the component to the Loader.") + u'\n'
            + prefix
            + Tr::tr("Check all uses of 'parent' inside the root element of the component.") + u'\n';

    if (!maybeId.isEmpty()) {
        comment += prefix
                   + Tr::tr("Rename all outer uses of the id \"%1\" to \"%2.item\".").arg(maybeId, loaderId) + u'\n';
    }
    for (const auto &id : nestedIds) {
        comment += prefix
                + Tr::tr("Rename all outer uses of the id \"%1\" to \"%2.item.%1\".").arg(id.first, loaderId) + u'\n';
    }

    return { { pos, pos }, comment.toUtf8() };
}

static TextEdits wrapIntoComponent(const Range &itemRange, const QString &componentId)
{
    const QString componentOpen = QString::fromLatin1("Component {\n"
                                                      "    id: %1\n")
                                          .arg(componentId);
    const QString componentClose = QString::fromLatin1("\n}\n");
    return { TextEdit{ { itemRange.start, itemRange.start }, componentOpen.toUtf8() },
             TextEdit{ { itemRange.end, itemRange.end }, componentClose.toUtf8() } };
}

static TextEdit addLoader(const Position &pos, const QString &loaderId, const QString &componentId)
{
    const QString loader = QString::fromLatin1("Loader {\n"
                                               "    id: %2\n"
                                               "    sourceComponent: %1\n"
                                               "}\n")
                                   .arg(componentId, loaderId);

    return { { pos, pos }, loader.toUtf8() };
}

static TextEdits exposeNestedIds(const QQmlJS::SourceLocation &openingBrace,
                                 const QList<std::pair<QString, QQmlJS::SourceLocation>> &nestedIds)
{
    const QLatin1StringView nestedIdPrefix("inner_");
    QString idAliases = QString::fromLatin1("\n");
    for (auto &id : nestedIds) {
        idAliases += QString::fromLatin1("property alias %1: %2%1\n").arg(id.first, nestedIdPrefix);
    }

    TextEdits edits;

    const auto posAfterLBrace =
            Position{ static_cast<unsigned int>(static_cast<int>(openingBrace.startLine - 1)),
                      static_cast<unsigned int>(
                              static_cast<int>(openingBrace.startColumn)) /* after { */ };
    // edit introducing property aliases
    edits.append(TextEdit{ { posAfterLBrace, posAfterLBrace }, idAliases.toUtf8() });

    // edits appending prefix "inner_" to each nested id
    for (auto &id : nestedIds) {
        const auto idPos =
                Position{ static_cast<unsigned int>(static_cast<int>(id.second.startLine - 1)),
                          static_cast<unsigned int>(static_cast<int>(id.second.startColumn - 1)) };
        edits.append(TextEdit{ { idPos, idPos }, nestedIdPrefix.toUtf8() });
    }
    return edits;
}

static TextEdits wrapInLoaderTextEdits(const QQmlLSUtils::ItemLocation &item)
{
    const auto generateId = [&item](const QString &base) -> QString {
        const auto ids = item.domItem.component().field(Fields::ids).keys();
        if (!ids.contains(base)) {
            return base;
        };
        int extraNumber = 1;
        for (; extraNumber < ids.size(); ++extraNumber) {
            QString id = base + QString::number(extraNumber);
            if (!ids.contains(id)) {
                return id;
            }
        }
        return base + QString::number(extraNumber);
    };

    const auto objId = item.domItem.idStr();
    const auto objName = objId.isEmpty() ? item.domItem.name() : objId;
    const QString componentId = generateId(QLatin1StringView("component_") + objName);
    const QString loaderId = generateId(QLatin1StringView("loader_") + objName);

    const auto itemRange = QQmlLSUtils::qmlLocationToLspLocation(
            QQmlLSUtils::Location::tryFrom(item.domItem.canonicalFilePath(),
                                           item.fileLocation->info().fullRegion, item.domItem)
                    .value_or(QQmlLSUtils::Location{}));

    QList<std::pair<QString, QQmlJS::SourceLocation>> nestedIds = collectNestedIds(item);
    if (!objId.isEmpty()) {
        // We expect the first found nested Id to be an object id.
        // Watch out for collectNestedIds changes
        Q_ASSERT(nestedIds.front().first == objId);
        nestedIds.removeFirst();
    }

    TextEdits edits;
    edits << todoComment(itemRange.start, loaderId, objId, nestedIds)
          << exposeNestedIds(item.fileLocation->info().regions[FileLocationRegion::LeftBraceRegion],
                             nestedIds)
          << wrapIntoComponent(itemRange, componentId)
          << addLoader(itemRange.end, loaderId, componentId);
    return edits;
}

static inline std::optional<QQmlLSUtils::ItemLocation>
qmlObjectDefinedAt(const QQmlLSUtils::ItemLocation &item)
{
    if (item.domItem.internalKind() != DomType::ScriptIdentifierExpression) {
        return std::nullopt;
    }
    auto parentObject = item.domItem.qmlObject();
    const auto objectLoc = FileLocations::treeOf(parentObject);

    if (item.fileLocation->info().fullRegion.begin() != objectLoc->info().fullRegion.begin()) {
        return std::nullopt;
    }

    return QQmlLSUtils::ItemLocation{ parentObject, objectLoc };
}

static CodeActions
wrapComponentInLoader(const OptionalVersionedTextDocumentIdentifier &textDocument,
                      const QQmlLSUtils::ItemLocation &item)
{
    if (item.domItem.internalKind() != DomType::QmlObject) {
        // If the current item is actually the identifier that defines a QML object,
        // treat it as if the object itself was selected
        if (auto qmlObject = qmlObjectDefinedAt(item)) {
            return wrapComponentInLoader(textDocument, *qmlObject);
        }
        // applicable only to objects
        return {};
    }
    if (item.domItem == item.domItem.component().field(Fields::objects).index(0)) {
        // not applicable for a root object
        return {};
    }
    if (item.domItem.canonicalPath().last() == Path::fromField(Fields::value)) {
        // not supported for the binding value, i.e. p: Item{}
        return {};
    }

    TextDocumentEdit textDocEdit{ textDocument, wrapInLoaderTextEdits(item) };

    WorkspaceEdit edit;
    edit.documentChanges = { textDocEdit };

    CodeAction action;
    action.kind = CodeActionKind::RefactorRewrite;
    action.title = Tr::tr("Wrap Component in Loader").toUtf8();
    action.edit = edit;
    return { action };
}

static CodeActions refactorings(const OptionalVersionedTextDocumentIdentifier &textDocument,
                                const QQmlLSUtils::ItemLocation &item)
{
    CodeActions codeActions;
    codeActions.append(wrapComponentInLoader(textDocument, item));
    return codeActions;
}

QQmlCodeActionSupport::QQmlCodeActionSupport(QmlLsp::QQmlCodeModelManager *model) : BaseT(model) { }

void QQmlCodeActionSupport::setupCapabilities(QLspSpecification::ServerCapabilities &caps)
{
    caps.codeActionProvider = true;
}

void QQmlCodeActionSupport::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerCodeActionRequestHandler(getRequestHandler());
}

void QQmlCodeActionSupport::process(QQmlCodeActionSupport::RequestPointerArgument request)
{
    CodeActions codeActions;
    codeActions.append(quickfixes(request->m_parameters.context.diagnostics));

    const auto &maybeDoc = tryOpenDocument(request->m_parameters.textDocument.uri);
    if (!maybeDoc.has_value()) {
        qCWarning(lsCodeActionSupport) << maybeDoc.error().message;
        return request->m_response.sendResponse(codeActions);
    }

    const auto &itemsFound = tryLocateItems(maybeDoc.value(), request->m_parameters.range.start);
    if (!itemsFound.has_value()) {
        qCWarning(lsCodeActionSupport) << itemsFound.error().message;
        return request->m_response.sendResponse(codeActions);
    }

    codeActions.append(refactorings(
            { request->m_parameters.textDocument, maybeDoc.value().snapshot.validDocVersion },
            itemsFound.value().front()));

    request->m_response.sendResponse(codeActions);
}

QT_END_NAMESPACE
