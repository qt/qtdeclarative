/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <qtest.h>

class tst_qmltc : public QObject
{
    Q_OBJECT

    bool isCacheDisabled() const
    {
        static bool isDisabled = []() { return qgetenv("QML_DISABLE_DISK_CACHE") == "1"_qba; }();
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
    void componentTypes();
    void deferredProperties();
    void gradients(); // QTBUG-102560
    void jsvalueAssignments();

    void signalHandlers();
    void jsFunctions();
    void changingBindings();
    void propertyAlias();
    void propertyAlias_external();
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
    void singletonUser();
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
};
