// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>

class tst_document_symbol_utils : public QObject
{
    Q_OBJECT
private:
    using DomItem = QQmlJS::Dom::DomItem;
    using SymbolsList = QList<QLspSpecification::DocumentSymbol>;
private:
    DomItem fakeQmlFileItem();
    SymbolsList expectedSymbolsOfFakeQmlFile();
    static inline QLspSpecification::DocumentSymbol
    fakeSymbol(QQmlJS::Dom::DomType type, SymbolsList &&children = SymbolsList())
    {
        QLspSpecification::DocumentSymbol fakeSymbol;
        fakeSymbol.name = domTypeToString(type).toUtf8();
        fakeSymbol.children = std::move(children);
        return fakeSymbol;
    }

private slots:
    void assembleSymbolsForQmlFile();
    void symbolNameOf();
    void symbolKindOf();
    void tryGetDetailOf();
    void reorganizeForOutlineView();
};
