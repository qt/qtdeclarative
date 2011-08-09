#ifndef QSGGEOMETRY_P_H
#define QSGGEOMETRY_P_H

#include "qsggeometry.h"

QT_BEGIN_NAMESPACE

class QSGGeometryData
{
public:
    virtual ~QSGGeometryData() {}

    static inline QSGGeometryData *data(const QSGGeometry *g) {
        return g->m_server_data;
    }

    static inline void install(const QSGGeometry *g, QSGGeometryData *data) {
        Q_ASSERT(!g->m_server_data);
        const_cast<QSGGeometry *>(g)->m_server_data = data;
    }

    static bool inline hasDirtyVertexData(const QSGGeometry *g) { return g->m_dirty_vertex_data; }
    static void inline clearDirtyVertexData(const QSGGeometry *g) { const_cast<QSGGeometry *>(g)->m_dirty_vertex_data = false; }

    static bool inline hasDirtyIndexData(const QSGGeometry *g) { return g->m_dirty_vertex_data; }
    static void inline clearDirtyIndexData(const QSGGeometry *g) { const_cast<QSGGeometry *>(g)->m_dirty_index_data = false; }

};

QT_END_NAMESPACE

#endif // QSGGEOMETRY_P_H
