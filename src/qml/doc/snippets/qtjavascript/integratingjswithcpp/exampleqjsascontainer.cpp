// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause


//! [qjs-as-container]

    class Cache : public QObject
    {
      Q_OBJECT
      QML_ELEMENT

      public:
        Q_INVOKABLE QJSValue lookup(const QString &key) {
          if (auto it = m_cache.constFind(key); it != m_cache.constEnd()) {
            return *it; // impicit conversion
          } else {
            return QJSValue::UndefinedValue; // implicit conversion
          }
        }

      QHash<QString, QString> m_cache;
    }

//! [qjs-as-container]
