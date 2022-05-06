/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

import QML

QtObject {
    default property list<Member> members

    property string file
    required property string name
    property string prototype
    property var exports: []
    property var exportMetaObjectRevisions: []
    property var interfaces: []
    property string attachedType
    property string valueType
    property string extension
    property bool isSingleton: false
    property bool isCreatable: name.length > 0
    property bool isComposite: false
    property bool hasCustomParser: false
    property string accessSemantics: "reference"
    property string defaultProperty
    property string parentProperty
}
