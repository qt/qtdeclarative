# Generated from tests.pro.

# special case begin
find_package(Qt6 REQUIRED Concurrent)
# special case end

add_subdirectory(auto)
# special case begin
# benchmarks not yet ported
# if(QT_CONFIG___contains___release)
#    add_subdirectory(benchmarks)
# endif()
# special case end
