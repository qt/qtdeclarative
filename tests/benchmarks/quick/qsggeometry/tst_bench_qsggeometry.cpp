// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QtCore/QList>
#include <QElapsedTimer>
#include <QtQuick/QSGGeometry>
#include <numeric>

class GeometryBenchmark : public QObject
{
    Q_OBJECT

private slots:
    void benchmarkVerticesInCache_data();
    void benchmarkVerticesInCache();
    void benchmarkVerticesInSystemMemory_data();
    void benchmarkVerticesInSystemMemory();

private:
    struct GeometryUpdateParams {
        int startingVertexPosition;
        int totalFramesOfAnimation;
        int vertexCountStep;
        bool useSetVertexCount;
    };
    template<typename YValueFunc>
    static void runVertexBenchmark(const GeometryUpdateParams& params,
                                     YValueFunc makeYValue);
};

// Compare allocate() and setVertexCount() for changing vertex count and
// updating vertices using cached data
//
// Benchmark results:
//
//                           Vertices   allocate()   setVertexCount()
//   i.MX6 800 MHz            2-17K  |   174 μs   |    0.75 μs
//                          20K-37K  |   579 μs   |    1.10 μs
//                          40K-57K  |   988 μs   |    1.57 μs
//
//   Intel i5-8265U 3.8Ghz    2-17K  |    25 μs   |    0.05 μs
//                          20K-37K  |    79 μs   |    0.06 μs
//                          40K-57K  |   145 μs   |    0.09 μs
//
//   AMD 7945HX 5.5GHz        2-17K  |     5 μs   |    0.02 μs
//                          20K-37K  |    17 μs   |    0.03 μs
//                          40K-57K  |    29 μs   |    0.05 μs
//
// This benchmark uses two caches:
//   - caching vertex data in a float array of y-values
//   - processor cache
//
// This test is simplistic and does not reflect the more likely case that
// there would be other threads doing work and causing processor-cache misses.
//
void GeometryBenchmark::benchmarkVerticesInCache_data()
{
    QTest::addColumn<bool>("useSetVertexCount");
    QTest::addColumn<int>("totalFramesOfAnimation");
    QTest::addColumn<int>("vertexCountStep");
    QTest::addColumn<int>("startingVertexPosition");

    QTest::newRow("allocate() grow from 2 verts")
        << false << 1000 << 17 << 2;
    QTest::newRow("allocate() grow from 20000 vert")
        << false << 1000 << 17 << 20'000;
    QTest::newRow("allocate() grow from 40000 verts")
        << false << 1000 << 17 << 40'000;
    QTest::newRow("setVertexCount() grow from 2 verts")
        << true  << 1000 << 17 << 0;
    QTest::newRow("setVertexCount() grow from 20000 verts")
        << true  << 1000 << 17 << 20'000;
    QTest::newRow("setVertexCount() grow from 40000 vert")
        << true  << 1000 << 17 << 40'000;
}

void GeometryBenchmark::benchmarkVerticesInCache()
{
    QFETCH(bool, useSetVertexCount);
    QFETCH(int, totalFramesOfAnimation);
    QFETCH(int, vertexCountStep);
    QFETCH(int, startingVertexPosition);

    auto makeYValue = [](int i) {
        return i/10.0f;
    };

    GeometryUpdateParams params{
        startingVertexPosition,
        totalFramesOfAnimation,
        vertexCountStep,
        useSetVertexCount
    };

    runVertexBenchmark(params, makeYValue);
}


// Compare uncached performance of allocate() and setVertexCount().
//
// This benchmark demonstrates the performance benefit of patch:
//    https://codereview.qt-project.org/c/qt/qtdeclarative/+/590520
// which implements QTBUG-126835 "Partial rendering of a geometry node"
//
// The goal of QTBUG-126835 is to provide an efficient means for changing the
// number of vertices while animating. Using allocate() to change the number
// of vertices requires updating all vertices, whereas setVertexCount() only
// requires updating new vertices.
//
// The test runs for 1000 frames (17 seconds of animation) and changes the
// number of vertices with each new frame.
//
// This benchmark elminates the benefit of CPU cache and is a good indicator
// of real world performance in a heavily multithreaded environment.
//
// Benchmark results:
//
//                          Vertices   allocate()   setVertexCount()
//   i.MX6 800 MHz            2-17K  |  1.7 msecs |       7 μs
//                          20K-37K  |  6.0 msecs |      10 μs
//                          40K-57K  | 10.1 msecs |      16 μs
//
//   Intel i5-8265U 3.8Ghz    2-17K  |    157 μs  |    0.21 μs
//                          20K-37K  |    654 μs  |    0.53 μs
//                          40K-57K  |   1133 μs  |    0.85 μs
//
//   AMD 7945HX 5.5GHz        2-17K  |     37 μs  |    0.16 μs
//                          20K-37K  |    147 μs  |    0.33 μs
//                          40K-57K  |    257 μs  |    0.29 μs
//
void GeometryBenchmark::benchmarkVerticesInSystemMemory_data()
{
    benchmarkVerticesInCache_data();
}

void GeometryBenchmark::benchmarkVerticesInSystemMemory()
{
    QFETCH(bool, useSetVertexCount);
    QFETCH(int, totalFramesOfAnimation);
    QFETCH(int, vertexCountStep);
    QFETCH(int, startingVertexPosition);

    const int totalVerts = startingVertexPosition
                           + (vertexCountStep * totalFramesOfAnimation);

    // Data is stored in an array of buffers and reads rotate through the
    // buffers so that each subsequent read is targeting a location in memory
    // that is not already cached. As a result, this tests the performance of
    // reading vertex data from system memory.

    std::array<std::vector<float>, 1000> data;
    data.fill(std::vector<float>(totalVerts));

    // Initialize each vector's element with the element index
    for (auto& arr : data)
        for (uint i = 0; i < arr.size(); ++i)
            arr[i] = i;

    // Index into the arrays
    size_t arrayIndex = 0;

    // Force cache misses by rolling backwards through the data arrays
    auto makeYValue = [&arrayIndex, &data](size_t vector_idx) -> float {
        float value = data[arrayIndex][vector_idx];

        arrayIndex = (arrayIndex == 0) ? data.size() - 1
                                       : arrayIndex - 1;
        return value;
    };

    GeometryUpdateParams params{
        startingVertexPosition,
        totalFramesOfAnimation,
        vertexCountStep,
        useSetVertexCount
    };

    runVertexBenchmark(params, makeYValue);
}

// This function is shared between the benchmarkVerticesInCache() and
// benchmarkVerticesInSystemMemory() benchmark functions.
//
// The function benchmarks the efficiency of updating vertex data in a
// QSGGeometry object, comparing the performance of using setVertexCount()
// versus allocate(). It measures the time taken to update vertices over
// multiple frames and ensures data integrity by verifying that the
// geometry's data matches expected values.
//
template<typename YValueFunc>
void GeometryBenchmark::runVertexBenchmark(const GeometryUpdateParams& params,
                                           YValueFunc makeYValue)
{
    int finalVertexCount = params.startingVertexPosition
                           + (params.vertexCountStep * params.totalFramesOfAnimation);

    // Create a collection to hold Y values for verifying data integrity
    QList<float> yValues(finalVertexCount);

    // Populate values collection with pseudo-data up to minimum vertices
    for (int i = 0; i < params.startingVertexPosition; ++i) {
        yValues[i] = makeYValue(i);
    }

    QSGGeometry geometry(QSGGeometry::defaultAttributes_Point2D(), 1, 0);

    // function to add data to geometry
    auto updateVertices = [&geometry, &makeYValue](int start, int end) {
        auto *pt = geometry.vertexDataAsPoint2D() + start;
        for (int i = start; i < end; ++i, ++pt) {
            pt->x = i;
            pt->y = makeYValue(i);
        }
    };

    int iterations = 0;
    QElapsedTimer timer;
    timer.start();
    do {
        for (int i = params.startingVertexPosition; i < finalVertexCount; i += params.vertexCountStep) {
            int newCount = qMin(finalVertexCount, i + params.vertexCountStep);

            // add data to values collection which will be added to geometry
            for (int j = i; j < newCount; ++j) {
                yValues[j] = makeYValue(j);
            }

            // set up geometry to have previous data and prepare for new data
            if (params.useSetVertexCount) {
                if (i == params.startingVertexPosition) {
                    // populate initial data
                    geometry.allocate(finalVertexCount);
                    updateVertices(0, params.startingVertexPosition);
                }

                // resizing does not invalidate previously allocated vertices
                geometry.setVertexCount(newCount);
            } else {
                geometry.allocate(newCount);
                // must repopulate because all vertices were invalidated by allocate()
                updateVertices(0, i);
            }

            // copy new data to geometry
            updateVertices(i, newCount);
        }
        ++iterations;
    } while (timer.elapsed() < 2);

    // Divide by the number of frames to get the time per frame
    qreal nanosecsPerTest = qreal(timer.nsecsElapsed()) / iterations;
    qreal nanosecs = nanosecsPerTest/params.totalFramesOfAnimation;
    QTest::setBenchmarkResult(nanosecs, QTest::WalltimeNanoseconds);

    // Verify that the geometry has expected data
    auto *pt = geometry.vertexDataAsPoint2D();
    for (int i = 0; i < finalVertexCount; ++i, ++pt) {
        QCOMPARE(pt->x, float(i));
        QCOMPARE(pt->y, yValues[i]);
    }
}

QTEST_MAIN(GeometryBenchmark)
#include "tst_bench_qsggeometry.moc"
