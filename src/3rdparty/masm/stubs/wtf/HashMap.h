/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
#ifndef HASHMAP_H
#define HASHMAP_H

#include <QtCore/qhash.h>

namespace WTF {

template<typename Key, typename Value>
class HashMap final : public QHash<Key, Value>
{
public:
    void add(const Key &k, const Value &v) { QHash<Key, Value>::insert(k, v); }
    Value get(const Key &k) { return QHash<Key, Value>::value(k); }
};

}

using WTF::HashMap;

#endif
