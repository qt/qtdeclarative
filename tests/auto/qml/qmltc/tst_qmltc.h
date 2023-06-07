// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>

using namespace Qt::StringLiterals;

class tst_qmltc : public QObject
{
    Q_OBJECT

    bool isCacheDisabled() const
    {
        static bool isDisabled = []() { return qgetenv("QML_DISABLE_DISK_CACHE") == "1"_ba; }();
        return isDisabled;
    }

public:
    tst_qmltc();

private slots:
    void initTestCase();

    void qmlNameConflictResolution();
    void helloWorld();
    void qtQuickIncludes();
    void enumerations();
    void methods();
    void properties();
    void ids();
    void importNamespace();
    void deferredProperties();
    void gradients(); // QTBUG-102560
    void jsvalueAssignments();
    void extensionTypeBindings();
    void visibleAliasMethods(); // QTBUG-103956
    void nonStandardIncludesInsideModule(); // QTBUG-104094
    void specialProperties();
    void regexpBindings();
    void aliasAssignments();
    void connections();

    void signalHandlers();
    void jsFunctions();
    void changingBindings();
    void propertyAlias();
    void propertyAlias_external();
    void propertyAliasAttribute();
    void complexAliases();
    void propertyChangeHandler();
    void nestedHelloWorld();
    void componentHelloWorld();
    void propertyReturningFunction();
    void listProperty();
    void listPropertiesWithTheSameName();
    void defaultProperty();
    void defaultPropertyCorrectSelection();
    void defaultAlias();
    void attachedProperty();
    void attachedPropertyObjectCreatedOnce();
    void groupedProperty();
    void groupedProperty_qquicktext();
    void localImport();
    void explicitLocalImport();
    void newPropertyBoundToOld();
    void oldPropertyBoundToNew();
    void nonLocalQmlPropertyBoundToAny();
    void localImportWithOnCompleted();
    void justAnimation();
    void justAnimationOnAlias();
    void behaviorAndAnimation();
    void behaviorAndAnimationOnAlias();
    void bindingsThroughIds();
    void contextHierarchy_rootBaseIsQml();
    void contextHierarchy_childBaseIsQml();
    void contextHierarchy_delegate();
    void contextHierarchy_nontrivial();
    void javascriptImport();
    void listView();
    void bindingOnValueType();
    void keyEvents();
    void privateProperties();
    void calqlatrBits(); // corner cases from calqlatr demo
    void trickyPropertyChangeAndSignalHandlers();
    void valueTypeListProperty();
    void translations();
    void repeaterCrash();
    void generalizedGroupedProperty();
    void appendToQQmlListProperty();
    void inlineComponents();
    void aliases();
    void inlineComponentsFromDifferentFiles();
    void singletons();
    void constSignalParameters();
    void cppNamespaces();
    void namespacedName();
    void checkExportsAreCompiling();
};
