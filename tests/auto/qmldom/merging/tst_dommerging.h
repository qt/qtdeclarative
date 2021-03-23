/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**/
#ifndef TST_DOMMERGING_H
#define TST_DOMMERGING_H
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlDom/private/qqmldomastdumper_p.h>

#include <QtTest/QtTest>
#include <QCborValue>
#include <QDebug>
#include <QLibraryInfo>

#include <memory>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT TestDomMerging : public QObject
{
    Q_OBJECT
public:
    static ErrorGroups myErrors()
    {
        static ErrorGroups res { { NewErrorGroup("tests"), NewErrorGroup("domitem") } };
        return res;
    }

private slots:
    void initTestCase()
    {
        QString baseDir = QLatin1String(QT_QMLTEST_DATADIR) + QLatin1String("/dommerging");
        QStringList qmltypeDirs =
                QStringList({ baseDir, QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath) });

        auto envPtr = std::shared_ptr<QQmlJS::Dom::DomEnvironment>(new QQmlJS::Dom::DomEnvironment(
                qmltypeDirs,
                DomEnvironment::Option::SingleThreaded | DomEnvironment::Option::NoDependencies));
        QQmlJS::Dom::DomItem env(envPtr);
        QVERIFY(env);
        QString testFile1 = baseDir + QLatin1String("/test1.qml");

        env.loadFile(
                testFile1, QString(),
                [this](Path, const DomItem &, const DomItem &newIt) { this->tFile = newIt; },
                LoadOption::DefaultLoad);
        env.loadFile(baseDir, QString(), {}, LoadOption::DefaultLoad);
        envPtr->loadPendingDependencies(env);

        QVERIFY(tFile);
        tFile = tFile.field(Fields::currentItem);
        QVERIFY(tFile);
    }

    void testReformat()
    {
        DomItem comp1 = tFile.field(Fields::components).key(QString()).index(0);
        QVERIFY(comp1);
        DomItem obj1 = comp1.field(Fields::objects).index(0);
        QVERIFY(obj1);
        DomItem width = obj1.field(Fields::bindings).key(QLatin1String("width")).index(0);
        DomItem w = obj1.bindings().key(QLatin1String("width"));
        QVERIFY(w.length() > 0);
        QCOMPARE(w.length(), 1);
        DomItem exp = width.field(Fields::value);
        if (std::shared_ptr<ScriptExpression> expPtr = exp.ownerAs<ScriptExpression>()) {
            QCOMPARE(expPtr->code(), u"{ height *3/4 }");
        }
        MutableDomItem myobj(obj1);
        PropertyDefinition pDef;
        pDef.name = QLatin1String("foo");
        pDef.typeName = QLatin1String("int");
        MutableDomItem propertyDef = myobj.addPropertyDef(pDef, AddOption::Overwrite);
        QVERIFY(propertyDef);
        MutableDomItem binding = myobj.addBinding(
                Binding(QLatin1String("foo"),
                        std::shared_ptr<ScriptExpression>(new ScriptExpression(
                                QLatin1String("42"),
                                ScriptExpression::ExpressionType::BindingExpression))),
                AddOption::Overwrite);
        QVERIFY(binding);
        MutableDomItem pInfo = myobj.field(Fields::propertyInfos).key(QLatin1String("foo"));
        // dumperToQDebug([pInfo](Sink s){ pInfo.dump(s); });
        QCOMPARE(propertyDef.item(), pInfo.field(Fields::propertyDefs).index(0).item());
        QCOMPARE(binding, pInfo.field(Fields::bindings).index(0));
    }

    void testFromScratch()
    {
        // QmlFile
        // add import
        // set prop
        // add prop
        // ubObj
    }

private:
    std::shared_ptr<DomEnvironment> envPtr;
    DomItem env;
    DomItem tFile;
};

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif
