// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlcompletionsupport_p.h"
#include "qqmllsutils_p.h"

#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/private/qduplicatetracker_p.h>
#include <QtCore/QRegularExpression>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

QT_BEGIN_NAMESPACE
using namespace QLspSpecification;
using namespace QQmlJS::Dom;
using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(complLog, "qt.languageserver.completions")

bool CompletionRequest::fillFrom(QmlLsp::OpenDocument doc, const Parameters &params,
                                 Response &&response)
{
    // do not call BaseRequest::fillFrom() to avoid taking the Mutex twice and getting an
    // inconsistent state.
    m_parameters = params;
    m_response = std::move(response);

    if (!doc.textDocument)
        return false;

    std::optional<int> targetVersion;
    {
        QMutexLocker l(doc.textDocument->mutex());
        targetVersion = doc.textDocument->version();
        code = doc.textDocument->toPlainText();
    }
    m_minVersion = (targetVersion ? *targetVersion : 0);

    return true;
}

void QmlCompletionSupport::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerCompletionRequestHandler(getRequestHandler());
    protocol->registerCompletionItemResolveRequestHandler(
            [](const QByteArray &, const CompletionItem &cParams,
               LSPResponse<CompletionItem> &&response) { response.sendResponse(cParams); });
}

QString QmlCompletionSupport::name() const
{
    return u"QmlCompletionSupport"_s;
}

void QmlCompletionSupport::setupCapabilities(
        const QLspSpecification::InitializeParams &,
        QLspSpecification::InitializeResult &serverCapabilities)
{
    QLspSpecification::CompletionOptions cOptions;
    if (serverCapabilities.capabilities.completionProvider)
        cOptions = *serverCapabilities.capabilities.completionProvider;
    cOptions.resolveProvider = false;
    cOptions.triggerCharacters = QList<QByteArray>({ QByteArray(".") });
    serverCapabilities.capabilities.completionProvider = cOptions;
}

void QmlCompletionSupport::process(RequestPointerArgument req)
{
    QmlLsp::OpenDocumentSnapshot doc =
            m_codeModel->snapshotByUrl(req->m_parameters.textDocument.uri);
    req->sendCompletions(doc);
}

QString CompletionRequest::urlAndPos() const
{
    return QString::fromUtf8(m_parameters.textDocument.uri) + u":"
            + QString::number(m_parameters.position.line) + u":"
            + QString::number(m_parameters.position.character);
}

// finds the filter string, the base (for fully qualified accesses) and the whole string
// just before pos in code
struct CompletionContextStrings
{
    CompletionContextStrings(QString code, qsizetype pos);

public:
    // line up until pos
    QStringView preLine() const
    {
        return QStringView(m_code).mid(m_lineStart, m_pos - m_lineStart);
    }
    // the part used to filter the completion (normally actual filtering is left to the client)
    QStringView filterChars() const
    {
        return QStringView(m_code).mid(m_filterStart, m_pos - m_filterStart);
    }
    // the base part (qualified access)
    QStringView base() const
    {
        return QStringView(m_code).mid(m_baseStart, m_filterStart - m_baseStart);
    }
    // if we are at line start
    bool atLineStart() const { return m_atLineStart; }

private:
    QString m_code; // the current code
    qsizetype m_pos = {}; // current position of the cursor
    qsizetype m_filterStart = {}; // start of the characters that are used to filter the suggestions
    qsizetype m_lineStart = {}; // start of the current line
    qsizetype m_baseStart = {}; // start of the dotted expression that ends at the cursor position
    bool m_atLineStart = {}; // if there are only spaces before base
};

CompletionContextStrings::CompletionContextStrings(QString code, qsizetype pos)
    : m_code(code), m_pos(pos)
{
    // computes the context just before pos in code.
    // After this code all the values of all the attributes should be correct (see above)
    // handle also letter or numbers represented a surrogate pairs?
    m_filterStart = m_pos;
    while (m_filterStart != 0) {
        QChar c = code.at(m_filterStart - 1);
        if (!c.isLetterOrNumber() && c != u'_')
            break;
        else
            --m_filterStart;
    }
    // handle spaces?
    m_baseStart = m_filterStart;
    while (m_baseStart != 0) {
        QChar c = code.at(m_baseStart - 1);
        if (c != u'.' || m_baseStart == 1)
            break;
        c = code.at(m_baseStart - 2);
        if (!c.isLetterOrNumber() && c != u'_')
            break;
        qsizetype baseEnd = --m_baseStart;
        while (m_baseStart != 0) {
            QChar c = code.at(m_baseStart - 1);
            if (!c.isLetterOrNumber() && c != u'_')
                break;
            else
                --m_baseStart;
        }
        if (m_baseStart == baseEnd)
            break;
    }
    m_atLineStart = true;
    m_lineStart = m_baseStart;
    while (m_lineStart != 0) {
        QChar c = code.at(m_lineStart - 1);
        if (c == u'\n' || c == u'\r')
            break;
        if (!c.isSpace())
            m_atLineStart = false;
        --m_lineStart;
    }
}

enum class TypeCompletionsType { None, Types, TypesAndAttributes };

enum class FunctionCompletion { None, Declaration };

enum class ImportCompletionType { None, Module, Version };

void CompletionRequest::sendCompletions(QmlLsp::OpenDocumentSnapshot &doc)
{
    QList<CompletionItem> res = completions(doc);
    m_response.sendResponse(res);
}

static QList<CompletionItem> importCompletions(DomItem &file, const CompletionContextStrings &ctx)
{
    // returns completions for import statements, ctx is supposed to be in an import statement
    QList<CompletionItem> res;
    ImportCompletionType importCompletionType = ImportCompletionType::None;
    QRegularExpression spaceRe(uR"(\W+)"_s);
    QList<QStringView> linePieces = ctx.preLine().split(spaceRe, Qt::SkipEmptyParts);
    qsizetype effectiveLength = linePieces.size()
            + ((!ctx.preLine().isEmpty() && ctx.preLine().last().isSpace()) ? 1 : 0);
    if (effectiveLength < 2) {
        CompletionItem comp;
        comp.label = "import";
        comp.kind = int(CompletionItemKind::Keyword);
        res.append(comp);
    }
    if (linePieces.isEmpty() || linePieces.first() != u"import")
        return res;
    if (effectiveLength == 2) {
        // the cursor is after the import, possibly in a partial module name
        importCompletionType = ImportCompletionType::Module;
    } else if (effectiveLength == 3) {
        if (linePieces.last() != u"as") {
            // the cursor is after the module, possibly in a partial version token (or partial as)
            CompletionItem comp;
            comp.label = "as";
            comp.kind = int(CompletionItemKind::Keyword);
            res.append(comp);
            importCompletionType = ImportCompletionType::Version;
        }
    }
    DomItem env = file.environment();
    if (std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>()) {
        switch (importCompletionType) {
        case ImportCompletionType::None:
            break;
        case ImportCompletionType::Module: {
            QDuplicateTracker<QString> modulesSeen;
            for (const QString &uri : envPtr->moduleIndexUris(env)) {
                QStringView base = ctx.base(); // if we allow spaces we should get rid of them
                if (uri.startsWith(base)) {
                    QStringList rest = uri.mid(base.size()).split(u'.');
                    if (rest.isEmpty())
                        continue;

                    const QString label = rest.first();
                    if (!modulesSeen.hasSeen(label)) {
                        CompletionItem comp;
                        comp.label = label.toUtf8();
                        comp.kind = int(CompletionItemKind::Module);
                        res.append(comp);
                    }
                }
            }
            break;
        }
        case ImportCompletionType::Version:
            if (ctx.base().isEmpty()) {
                for (int majorV :
                     envPtr->moduleIndexMajorVersions(env, linePieces.at(1).toString())) {
                    CompletionItem comp;
                    comp.label = QString::number(majorV).toUtf8();
                    comp.kind = int(CompletionItemKind::Constant);
                    res.append(comp);
                }
            } else {
                bool hasMajorVersion = ctx.base().endsWith(u'.');
                int majorV = -1;
                if (hasMajorVersion)
                    majorV = ctx.base().mid(0, ctx.base().size() - 1).toInt(&hasMajorVersion);
                if (!hasMajorVersion)
                    break;
                if (std::shared_ptr<ModuleIndex> mIndex =
                            envPtr->moduleIndexWithUri(env, linePieces.at(1).toString(), majorV)) {
                    for (int minorV : mIndex->minorVersions()) {
                        CompletionItem comp;
                        comp.label = QString::number(minorV).toUtf8();
                        comp.kind = int(CompletionItemKind::Constant);
                        res.append(comp);
                    }
                }
            }
            break;
        }
    }
    return res;
}

static QList<CompletionItem> idsCompletions(DomItem component)
{
    qCDebug(complLog) << "adding ids completions";
    QList<CompletionItem> res;
    for (const QString &k : component.field(Fields::ids).keys()) {
        CompletionItem comp;
        comp.label = k.toUtf8();
        comp.kind = int(CompletionItemKind::Value);
        res.append(comp);
    }
    return res;
}

static QList<CompletionItem> bindingsCompletions(DomItem &containingObject)
{
    // returns valid bindings completions (i.e. reachable properties and signal handlers)
    QList<CompletionItem> res;
    qCDebug(complLog) << "binding completions";
    containingObject.visitPrototypeChain(
            [&res](DomItem &it) {
                qCDebug(complLog) << "prototypeChain" << it.internalKindStr() << it.canonicalPath();
                if (const QmlObject *itPtr = it.as<QmlObject>()) {
                    // signal handlers
                    auto methods = itPtr->methods();
                    auto it = methods.cbegin();
                    while (it != methods.cend()) {
                        if (it.value().methodType == MethodInfo::MethodType::Signal) {
                            CompletionItem comp;
                            QString signal = it.key();
                            comp.label =
                                    (u"on"_s + signal.at(0).toUpper() + signal.mid(1)).toUtf8();
                            res.append(comp);
                        }
                        ++it;
                    }
                    // properties that can be bound
                    auto pDefs = itPtr->propertyDefs();
                    for (auto it2 = pDefs.keyBegin(); it2 != pDefs.keyEnd(); ++it2) {
                        qCDebug(complLog) << "adding property" << *it2;
                        CompletionItem comp;
                        comp.label = it2->toUtf8();
                        comp.insertText = (*it2 + u": "_s).toUtf8();
                        comp.kind = int(CompletionItemKind::Property);
                        res.append(comp);
                    }
                }
                return true;
            },
            VisitPrototypesOption::Normal);
    return res;
}

static QList<CompletionItem> reachableSymbols(DomItem &context, const CompletionContextStrings &ctx,
                                              TypeCompletionsType typeCompletionType,
                                              FunctionCompletion completeMethodCalls)
{
    // returns completions for the reachable types or attributes from context
    QList<CompletionItem> res;
    QMap<CompletionItemKind, QSet<QString>> symbols;
    QSet<quintptr> visited;
    QList<Path> visitedRefs;
    auto addLocalSymbols = [&res, typeCompletionType, completeMethodCalls, &symbols](DomItem &el) {
        switch (typeCompletionType) {
        case TypeCompletionsType::None:
            return false;
        case TypeCompletionsType::Types:
            switch (el.internalKind()) {
            case DomType::ImportScope: {
                const QSet<QString> localSymbols = el.localSymbolNames(
                        LocalSymbolsType::QmlTypes | LocalSymbolsType::Namespaces);
                qCDebug(complLog) << "adding local symbols of:" << el.internalKindStr()
                                  << el.canonicalPath() << localSymbols;
                symbols[CompletionItemKind::Class] += localSymbols;
                break;
            }
            default: {
                qCDebug(complLog) << "skipping local symbols for non type" << el.internalKindStr()
                                  << el.canonicalPath();
                break;
            }
            }
            break;
        case TypeCompletionsType::TypesAndAttributes:
            auto localSymbols = el.localSymbolNames(LocalSymbolsType::All);
            if (const QmlObject *elPtr = el.as<QmlObject>()) {
                auto methods = elPtr->methods();
                auto it = methods.cbegin();
                while (it != methods.cend()) {
                    localSymbols.remove(it.key());
                    if (completeMethodCalls == FunctionCompletion::Declaration) {
                        QStringList parameters;
                        for (const MethodParameter &pInfo : std::as_const(it->parameters)) {
                            QStringList param;
                            if (!pInfo.typeName.isEmpty())
                                param << pInfo.typeName;
                            if (!pInfo.name.isEmpty())
                                param << pInfo.name;
                            if (pInfo.defaultValue) {
                                param << u"= " + pInfo.defaultValue->code();
                            }
                            parameters.append(param.join(u' '));
                        }

                        QString commentsStr;

                        if (!it->comments.regionComments.isEmpty()) {
                            for (const Comment &c : it->comments.regionComments[QString()].preComments) {
                                commentsStr += c.rawComment().toString().trimmed() + u'\n';
                            }
                        }

                        CompletionItem comp;
                        comp.documentation =
                                u"%1%2(%3)"_s.arg(commentsStr, it.key(), parameters.join(u", "))
                                        .toUtf8();
                        comp.label = (it.key() + u"()").toUtf8();
                        comp.kind = int(CompletionItemKind::Function);

                        if (it->typeName.isEmpty())
                            comp.detail = "returns void";
                        else
                            comp.detail = (u"returns "_s + it->typeName).toUtf8();

                        // Only append full bracket if there are no parameters
                        if (it->parameters.isEmpty())
                            comp.insertText = comp.label;
                        else
                            // add snippet support?
                            comp.insertText = (it.key() + u"(").toUtf8();

                        res.append(comp);
                    }
                    ++it;
                }
            }
            qCDebug(complLog) << "adding local symbols of:" << el.internalKindStr()
                              << el.canonicalPath() << localSymbols;
            symbols[CompletionItemKind::Field] += localSymbols;
            break;
        }
        return true;
    };
    if (ctx.base().isEmpty()) {
        if (typeCompletionType != TypeCompletionsType::None) {
            qCDebug(complLog) << "adding symbols reachable from:" << context.internalKindStr()
                              << context.canonicalPath();
            DomItem it = context.proceedToScope();
            it.visitScopeChain(addLocalSymbols, LookupOption::Normal, &defaultErrorHandler,
                               &visited, &visitedRefs);
        }
    } else {
        QList<QStringView> baseItems = ctx.base().split(u'.', Qt::SkipEmptyParts);
        Q_ASSERT(!baseItems.isEmpty());
        auto addReachableSymbols = [&visited, &visitedRefs, &addLocalSymbols](Path,
                                                                              DomItem &it) -> bool {
            qCDebug(complLog) << "adding directly accessible symbols of" << it.internalKindStr()
                              << it.canonicalPath();
            it.visitDirectAccessibleScopes(addLocalSymbols, VisitPrototypesOption::Normal,
                                           &defaultErrorHandler, &visited, &visitedRefs);
            return true;
        };
        Path toSearch = Paths::lookupSymbolPath(ctx.base().toString().chopped(1));
        context.resolve(toSearch, addReachableSymbols, &defaultErrorHandler);
        // add attached types? technically we should...
    }
    for (auto symbolKinds = symbols.constBegin(); symbolKinds != symbols.constEnd();
         ++symbolKinds) {
        for (auto symbol = symbolKinds.value().constBegin();
             symbol != symbolKinds.value().constEnd(); ++symbol) {
            CompletionItem comp;
            comp.label = symbol->toUtf8();
            comp.kind = int(symbolKinds.key());
            res.append(comp);
        }
    }
    return res;
}

QList<CompletionItem> CompletionRequest::completions(QmlLsp::OpenDocumentSnapshot &doc) const
{
    QList<CompletionItem> res;
    if (!doc.validDoc) {
        qCWarning(complLog) << "No valid document for completions  for "
                            << QString::fromUtf8(m_parameters.textDocument.uri);
        // try to add some import and global completions?
        return res;
    }
    if (!doc.docVersion || *doc.docVersion < m_minVersion) {
        qCWarning(complLog) << "sendCompletions on older doc version";
    } else if (!doc.validDocVersion || *doc.validDocVersion < m_minVersion) {
        qCWarning(complLog) << "using outdated valid doc, position might be incorrect";
    }
    DomItem file = doc.validDoc.fileObject(QQmlJS::Dom::GoTo::MostLikely);
    // clear reference cache to resolve latest versions (use a local env instead?)
    if (std::shared_ptr<DomEnvironment> envPtr = file.environment().ownerAs<DomEnvironment>())
        envPtr->clearReferenceCache();
    qsizetype pos = QQmlLSUtils::textOffsetFrom(code, m_parameters.position.line,
                                                m_parameters.position.character);
    CompletionContextStrings ctx(code, pos);
    auto itemsFound = QQmlLSUtils::itemsFromTextLocation(file, m_parameters.position.line,
                                                         m_parameters.position.character
                                                                 - ctx.filterChars().size());
    if (itemsFound.size() > 1) {
        QStringList paths;
        for (auto &it : itemsFound)
            paths.append(it.domItem.canonicalPath().toString());
        qCWarning(complLog) << "Multiple elements of " << urlAndPos()
                            << " at the same depth:" << paths << "(using first)";
    }
    DomItem currentItem;
    if (!itemsFound.isEmpty())
        currentItem = itemsFound.first().domItem;
    qCDebug(complLog) << "Completion at " << urlAndPos() << " " << m_parameters.position.line << ":"
                      << m_parameters.position.character << "offset:" << pos
                      << "base:" << ctx.base() << "filter:" << ctx.filterChars()
                      << "lastVersion:" << (doc.docVersion ? (*doc.docVersion) : -1)
                      << "validVersion:" << (doc.validDocVersion ? (*doc.validDocVersion) : -1)
                      << "in" << currentItem.internalKindStr() << currentItem.canonicalPath();
    DomItem containingObject = currentItem.qmlObject();
    TypeCompletionsType typeCompletionType = TypeCompletionsType::None;
    FunctionCompletion methodCompletion = FunctionCompletion::Declaration;

    if (!containingObject) {
        methodCompletion = FunctionCompletion::None;
        // global completions
        if (ctx.atLineStart()) {
            if (ctx.base().isEmpty()) {
                {
                    CompletionItem comp;
                    comp.label = "pragma";
                    comp.kind = int(CompletionItemKind::Keyword);
                    res.append(comp);
                }
            }
            typeCompletionType = TypeCompletionsType::Types;
        }
        // Import completion
        res += importCompletions(file, ctx);
    } else {
        methodCompletion = FunctionCompletion::Declaration;
        bool addIds = false;

        if (ctx.atLineStart() && currentItem.internalKind() != DomType::ScriptExpression
            && currentItem.internalKind() != DomType::List) {
            // add bindings
            methodCompletion = FunctionCompletion::None;
            if (ctx.base().isEmpty()) {
                for (const QStringView &s : std::array<QStringView, 5>(
                             { u"property", u"readonly", u"default", u"signal", u"function" })) {
                    CompletionItem comp;
                    comp.label = s.toUtf8();
                    comp.kind = int(CompletionItemKind::Keyword);
                    res.append(comp);
                }
                res += bindingsCompletions(containingObject);
                typeCompletionType = TypeCompletionsType::Types;
            } else {
                // handle value types later with type expansion
                typeCompletionType = TypeCompletionsType::TypesAndAttributes;
            }
        } else {
            addIds = true;
            typeCompletionType = TypeCompletionsType::TypesAndAttributes;
        }
        if (addIds) {
            res += idsCompletions(containingObject.component());
        }
    }

    DomItem context = containingObject;
    if (!context)
        context = file;
    // adds types and attributes
    res += reachableSymbols(context, ctx, typeCompletionType, methodCompletion);
    return res;
}
QT_END_NAMESPACE
