cmake_minimum_required(VERSION 3.0.0)
project(test_cereal VERSION 0.1.0)

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_RELEASE "-std=c++11 -g -fPIC")

find_package(Boost COMPONENTS serialization)

if(Boost_FOUND)
  include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
endif(Boost_FOUND)

set(Boost_LIBRARIES boost_system boost_filesystem)

set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/3rdparty)

set(UOS_DIR /home/xiaolinz/uisee_src/slam_store/vslam_store/uos/src)
set(LSB_RELEASE 16.04)
set(LIBDIR lib)

# opencv
include_directories(SYSTEM ${THIRD_PARTY_DIR}/opencv33/16.04/include)
link_directories(${THIRD_PARTY_DIR}/opencv33/16.04/lib)
set(OpenCV_LIBS opencv_core opencv_highgui opencv_calib3d opencv_features2d opencv_imgproc opencv_imgcodecs opencv_video)

find_package(OpenCV 3 REQUIRED)

# CUDA
link_directories(${THIRD_PARTY_DIR}/cuda-9.0/16.04/lib)
set(CUDA_LIBS cudart )

# cereal
include_directories(SYSTEM ${THIRD_PARTY_DIR}/cereal/include)

# sqlite
include_directories(SYSTEM ${THIRD_PARTY_DIR}/sqlite3/include)
link_directories(${THIRD_PARTY_DIR}/sqlite3 )

# flatbuffer
include_directories(SYSTEM ${THIRD_PARTY_DIR}/flatbffer/include)
link_directories(${THIRD_PARTY_DIR}/flatbuffer/lib )

# shapelib
include_directories(SYSTEM ${THIRD_PARTY_DIR}/shapelib/include)
link_directories(${THIRD_PARTY_DIR}/shapelib/lib )

# yas
include_directories(SYSTEM ${THIRD_PARTY_DIR}/yas)

# rapidjson
include_directories(SYSTEM ${THIRD_PARTY_DIR}/rapidjson)

include_directories(
  include 
  ${THIRD_PARTY_DIR}
  ${CMAKE_HOME_DIRECTORY}
  ${CMAKE_HOME_DIRECTORY}/data
)

add_library(${PROJECT_NAME} SHARED
  src/parson.cpp
)

# add_executable(test_cereal cereal_test.cpp)
# target_link_libraries(${PROJECT_NAME} PUBLIC
#     sqlite3
#     libpthread.so
#     libdl.so
#     pthread
# )

# add_executable(test_yas yas_test.cpp)
# target_link_libraries(test_yas PUBLIC
#     ${OpenCV_LIBS}
#     ${CUDA_LIBS}
#     sqlite3
#     libpthread.so
#     libdl.so
#     pthread
# )

# add_executable(mat_test mat_test.cpp)
# target_link_libraries(mat_test PUBLIC
#     ${OpenCV_LIBS}
#     ${CUDA_LIBS}
#     libpthread.so
#     libdl.so
#     pthread
# )

# add_executable(flatbuffer_test flatbuffer_test.cpp)
# target_link_libraries(flatbuffer_test PUBLIC
#     libflatbuffers.a
#     libpthread.so
#     libdl.so
#     pthread
# )

# add_executable(struct_size struct_size_test.cpp)
# target_link_libraries(struct_size PUBLIC

#     libdl.so
#     pthread
# )

# add_executable(cut_image opencv_cut_image_test.cpp)
# target_link_libraries(cut_image PUBLIC
#     ${OpenCV_LIBS}
#     # ${CUDA_LIBS}
#     ${Boost_LIBRARIES}
#     libpthread.so
#     libdl.so
#     pthread
# )

# add_executable(polyfit polyfit.cpp)
# target_link_libraries(polyfit PUBLIC
#     ${OpenCV_LIBS}
#     # ${CUDA_LIBS}
#     libshp.so
#     ${Boost_LIBRARIES}
#     libpthread.so
#     libdl.so
#     pthread
# )


set(OTHER_THIRD_PARTY_DIR /home/xiaolinz/uisee_src/topo_source/road_topo/3rdparty)

# geos
include_directories(SYSTEM ${OTHER_THIRD_PARTY_DIR}/geos/include)
link_directories(${OTHER_THIRD_PARTY_DIR}/geos/lib )

# gdal
include_directories(SYSTEM ${OTHER_THIRD_PARTY_DIR}/gdal2.4/include)
link_directories(${OTHER_THIRD_PARTY_DIR}/gdal2.4/lib )


add_executable(shp src/shape_file.cpp)
target_link_libraries(shp PUBLIC
    # ${OpenCV_LIBS}
    # ${CUDA_LIBS}

    ${PROJECT_NAME}
    libgeos.so
    # libgeos_c.so
    libgdal.so
    ${Boost_LIBRARIES}
    libpthread.so
    libdl.so
    pthread
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
