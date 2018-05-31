cmake_minimum_required(VERSION 3.3)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include("ext/hunter/HunterGate.cmake")
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.12.40.tar.gz"
    SHA1 "0ccc3f3fd571676a1804723984598f9f90a4d6bc"
)

# Add HDT
# We may want to declare hdt as a library
include_directories(
        deps/hdt/libcds-v1.0.12/includes/
        deps/hdt/hdt-lib/include/
        deps/hdt/hdt-lib/src/
)
file(GLOB_RECURSE HDT_FILES
        "deps/hdt/hdt-lib/src/*.h"
        "deps/hdt/hdt-lib/src/*.cpp"
)

# Build settings
set(PROJECT_NAME_STR cobra)
project(${PROJECT_NAME_STR})

set(SOURCE_FILE_EVALUATE
        #src/test/cpp/performance.cc
        src/test/cpp/bear.cc
        src/main/cpp/evaluate/Experimenter.h src/main/cpp/evaluate/Experimenter.cc
#        src/main/cpp/evaluate/BulkInserter.cc src/main/cpp/evaluate/BulkInserter.h
        )
set(SOURCE_FILE_QUERY_VERSION_MATERIALIZED src/main/cpp/query_version_materialized.cc)
set(SOURCE_FILE_QUERY_DELTA_MATERIALIZED src/main/cpp/query_delta_materialized.cc)
set(SOURCE_FILE_QUERY_VERSION src/main/cpp/query_version.cc)
set(SOURCE_FILE_INSERT src/main/cpp/insert.cc)
set(SOURCE_FILE_REVERSE src/main/cpp/reverse.cc)
set(SOURCE_FILE_COPY src/main/cpp/copy.cc)

set(COMMON_FILES
        src/main/cpp/controller/controller.cc src/main/cpp/controller/controller.h
        src/main/cpp/patch/triple.cc src/main/cpp/patch/triple.h
        src/main/cpp/patch/triple_store.cc src/main/cpp/patch/triple_store.h
        src/main/cpp/patch/patch_element.cc src/main/cpp/patch/patch_element.h
        src/main/cpp/patch/patch.cc src/main/cpp/patch/patch.h
        src/main/cpp/patch/patch_tree_value.cc src/main/cpp/patch/patch_tree_value.h
        src/main/cpp/patch/patch_tree_deletion_value.cc src/main/cpp/patch/patch_tree_deletion_value.h
        src/main/cpp/patch/patch_tree_addition_value.cc src/main/cpp/patch/patch_tree_addition_value.h
        src/main/cpp/patch/patch_tree_key_comparator.cc src/main/cpp/patch/patch_tree_key_comparator.h
        src/main/cpp/patch/patch_tree.cc src/main/cpp/patch/patch_tree.h
        src/main/cpp/patch/patch_tree_iterator.cc src/main/cpp/patch/patch_tree_iterator.h
        src/main/cpp/patch/triple_iterator.cc src/main/cpp/patch/triple_iterator.h
        src/main/cpp/patch/positioned_triple_iterator.cc src/main/cpp/patch/positioned_triple_iterator.h
        src/main/cpp/patch/patch.h
        src/main/cpp/dictionary/dictionary_manager.cc src/main/cpp/dictionary/dictionary_manager.h
        src/main/cpp/snapshot/snapshot_manager.cc src/main/cpp/snapshot/snapshot_manager.h
        src/main/cpp/snapshot/vector_triple_iterator.cc src/main/cpp/snapshot/vector_triple_iterator.h
        src/main/cpp/controller/snapshot_patch_iterator_triple_id.cc src/main/cpp/controller/snapshot_patch_iterator_triple_id.h
        src/main/cpp/patch/patch_tree_manager.cc src/main/cpp/patch/patch_tree_manager.h
        src/main/cpp/snapshot/combined_triple_iterator.cc src/main/cpp/snapshot/combined_triple_iterator.h
        src/main/cpp/patch/patch_element_comparator.cc src/main/cpp/patch/patch_element_comparator.h
        src/main/cpp/simpleprogresslistener.cc src/main/cpp/simpleprogresslistener.h
        src/main/cpp/controller/patch_builder.cc src/main/cpp/controller/patch_builder.h
        src/main/cpp/controller/patch_builder_streaming.cc src/main/cpp/controller/patch_builder_streaming.h
        src/main/cpp/controller/triple_delta_iterator.cc src/main/cpp/controller/triple_delta_iterator.h
        src/main/cpp/controller/triple_versions_iterator.cc src/main/cpp/controller/triple_versions_iterator.h
        src/main/cpp/patch/patch_element_iterator.cc src/main/cpp/patch/patch_element_iterator.h
        )

add_executable(${PROJECT_NAME_STR}-evaluate ${HDT_FILES} ${COMMON_FILES} ${SOURCE_FILE_EVALUATE})

# Add Kyoto Cabinet
find_library(KYOTO_SHARED_LIBRARY kyotocabinet PATHS ${KC_ROOT}/lib)
find_path(KYOTO_INCLUDE_DIR kchashdb.h PATHS ${KC_ROOT}/include)
if (KYOTO_SHARED_LIBRARY MATCHES KYOTO_LIBRARY-NOTFOUND)
    message("FATAL_ERROR Kyoto Cabinet not found!")
    set(Kyoto-NOTFOUND 1)
    set(Kyoto-FOUND)
else (KYOTO_SHARED_LIBRARY MATCHES KYOTO_LIBRARY-NOTFOUND)
    if (KYOTO_INCLUDE_DIR MATCHES KYOTO_INCLUDE_DIR-NOTFOUND)
        message("FATAL_ERROR Kyoto Cabinet not found!")
        set(Kyoto-NOTFOUND 1)
        set(Kyoto-FOUND)
    else (KYOTO_INCLUDE_DIR MATCHES KYOTO_INCLUDE_DIR-NOTFOUND)
        set(Kyoto-FOUND 1)
        set(Kyoto-NOTFOUND)
        message(STATUS "Kyoto Cabinet Found")
        # get the static version
        string(REGEX REPLACE "\\${CMAKE_SHARED_LIBRARY_SUFFIX}$" ".a" KYOTO_STATIC_LIBRARY ${KYOTO_SHARED_LIBRARY})
        message(STATUS "Kyoto Cabinet: ${KYOTO_STATIC_LIBRARY}" )
        target_link_libraries(${PROJECT_NAME_STR}-evaluate ${KYOTO_SHARED_LIBRARY})
        include_directories(${KYOTO_INCLUDE_DIR})
        add_definitions(-DHAS_KYOTO)
    endif (KYOTO_INCLUDE_DIR MATCHES KYOTO_INCLUDE_DIR-NOTFOUND)
endif (KYOTO_SHARED_LIBRARY MATCHES KYOTO_LIBRARY-NOTFOUND)

if (Kyoto-FOUND)
    add_definitions(-DHAVE_KYOTO)
endif()

# Add pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME_STR}-evaluate Threads::Threads)

# Add zlib
hunter_add_package(ZLIB)
find_package(ZLIB CONFIG REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME_STR}-evaluate ${ZLIB_LIBRARIES})

# Add Boost
hunter_add_package(Boost COMPONENTS iostreams regex filesystem)
find_package(Boost CONFIG REQUIRED COMPONENTS iostreams regex filesystem)
target_link_libraries(${PROJECT_NAME_STR}-evaluate Boost::iostreams Boost::regex Boost::filesystem)

# 3rdparty Libraries
set(EXT_PROJECTS_DIR ${PROJECT_SOURCE_DIR}/ext)
#include(CMakeLists-External.txt)

# Add query version materialized executable
add_executable(${PROJECT_NAME_STR}-query-version-materialized ${HDT_FILES} ${COMMON_FILES} ${SOURCE_FILE_QUERY_VERSION_MATERIALIZED})
target_link_libraries(${PROJECT_NAME_STR}-query-version-materialized ${LIBSTDCXX})
target_link_libraries(${PROJECT_NAME_STR}-query-version-materialized ${KYOTO_SHARED_LIBRARY})
target_link_libraries(${PROJECT_NAME_STR}-query-version-materialized ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${PROJECT_NAME_STR}-query-version-materialized ${ZLIB_LIBRARIES})
target_link_libraries(${PROJECT_NAME_STR}-query-version-materialized Boost::iostreams)
target_link_libraries(${PROJECT_NAME_STR}-query-version-materialized Boost::filesystem)
target_link_libraries(${PROJECT_NAME_STR}-query-version-materialized Threads::Threads)

# Add query delta materialized executable
add_executable(${PROJECT_NAME_STR}-query-delta-materialized ${HDT_FILES} ${COMMON_FILES} ${SOURCE_FILE_QUERY_DELTA_MATERIALIZED})
target_link_libraries(${PROJECT_NAME_STR}-query-delta-materialized ${LIBSTDCXX})
target_link_libraries(${PROJECT_NAME_STR}-query-delta-materialized ${KYOTO_SHARED_LIBRARY})
target_link_libraries(${PROJECT_NAME_STR}-query-delta-materialized ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${PROJECT_NAME_STR}-query-delta-materialized ${ZLIB_LIBRARIES})
target_link_libraries(${PROJECT_NAME_STR}-query-delta-materialized Boost::iostreams)
target_link_libraries(${PROJECT_NAME_STR}-query-delta-materialized Boost::filesystem)
target_link_libraries(${PROJECT_NAME_STR}-query-delta-materialized Threads::Threads)

# Add query version executable
add_executable(${PROJECT_NAME_STR}-query-version ${HDT_FILES} ${COMMON_FILES} ${SOURCE_FILE_QUERY_VERSION})
target_link_libraries(${PROJECT_NAME_STR}-query-version ${LIBSTDCXX})
target_link_libraries(${PROJECT_NAME_STR}-query-version ${KYOTO_SHARED_LIBRARY})
target_link_libraries(${PROJECT_NAME_STR}-query-version ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${PROJECT_NAME_STR}-query-version ${ZLIB_LIBRARIES})
target_link_libraries(${PROJECT_NAME_STR}-query-version Boost::iostreams)
target_link_libraries(${PROJECT_NAME_STR}-query-version Boost::filesystem)
target_link_libraries(${PROJECT_NAME_STR}-query-version Threads::Threads)

# Add insert executable
add_executable(${PROJECT_NAME_STR}-insert ${HDT_FILES} ${COMMON_FILES} ${SOURCE_FILE_INSERT})
target_link_libraries(${PROJECT_NAME_STR}-insert ${LIBSTDCXX})
target_link_libraries(${PROJECT_NAME_STR}-insert ${KYOTO_SHARED_LIBRARY})
target_link_libraries(${PROJECT_NAME_STR}-insert ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${PROJECT_NAME_STR}-insert ${ZLIB_LIBRARIES})
target_link_libraries(${PROJECT_NAME_STR}-insert Boost::iostreams)
target_link_libraries(${PROJECT_NAME_STR}-insert Boost::filesystem)
target_link_libraries(${PROJECT_NAME_STR}-insert Threads::Threads)

# Add copy executable
add_executable(${PROJECT_NAME_STR}-copy ${HDT_FILES} ${COMMON_FILES} ${SOURCE_FILE_COPY})
target_link_libraries(${PROJECT_NAME_STR}-copy ${LIBSTDCXX})
target_link_libraries(${PROJECT_NAME_STR}-copy ${KYOTO_SHARED_LIBRARY})
target_link_libraries(${PROJECT_NAME_STR}-copy ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${PROJECT_NAME_STR}-copy ${ZLIB_LIBRARIES})
target_link_libraries(${PROJECT_NAME_STR}-copy Boost::iostreams)
target_link_libraries(${PROJECT_NAME_STR}-copy Boost::filesystem)
target_link_libraries(${PROJECT_NAME_STR}-copy Threads::Threads)

# Add reverse executable
add_executable(${PROJECT_NAME_STR}-reverse ${HDT_FILES} ${COMMON_FILES} ${SOURCE_FILE_REVERSE})
target_link_libraries(${PROJECT_NAME_STR}-reverse ${LIBSTDCXX})
target_link_libraries(${PROJECT_NAME_STR}-reverse ${KYOTO_SHARED_LIBRARY})
target_link_libraries(${PROJECT_NAME_STR}-reverse ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${PROJECT_NAME_STR}-reverse ${ZLIB_LIBRARIES})
target_link_libraries(${PROJECT_NAME_STR}-reverse Boost::iostreams)
target_link_libraries(${PROJECT_NAME_STR}-reverse Boost::filesystem)
target_link_libraries(${PROJECT_NAME_STR}-reverse Threads::Threads)

