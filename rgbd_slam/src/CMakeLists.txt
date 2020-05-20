
# 增加PCL库的依赖
FIND_PACKAGE( PCL REQUIRED COMPONENTS common io visualization filters)
list(REMOVE_ITEM PCL_LIBRARIES "vtkproj4") # use this in Ubuntu 16.04

# 增加opencv的依赖
FIND_PACKAGE( OpenCV REQUIRED )

# 添加头文件和库文件
ADD_DEFINITIONS( ${PCL_DEFINITIONS} )
INCLUDE_DIRECTORIES( ${PCL_INCLUDE_DIRS}  )
LINK_LIBRARIES( ${PCL_LIBRARY_DIRS} )

# 添加g2o的依赖
# 因为g2o不是常用库，要添加它的findg2o.cmake文件
LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules )
SET( G2O_ROOT /usr/local/include/g2o )
FIND_PACKAGE( G2O )
# CSparse
FIND_PACKAGE( CSparse )
message( "============ ${CSPARSE_INCLUDE_DIR} ")
INCLUDE_DIRECTORIES( ${G2O_INCLUDE_DIR} ${CSPARSE_INCLUDE_DIR} )

# 生成可执行文件
ADD_EXECUTABLE( main main.cpp )

ADD_EXECUTABLE( generate_pointcloud generatePointCloud.cpp )
TARGET_LINK_LIBRARIES( generate_pointcloud 
    ${OpenCV_LIBS} 
    ${PCL_LIBRARIES} 
    )

ADD_LIBRARY( slambase slamBase.cpp )
TARGET_LINK_LIBRARIES( slambase
    ${OpenCV_LIBS} 
    ${PCL_LIBRARIES} )

# 特征检测 
ADD_EXECUTABLE( detectFeatures detectFeatures.cpp )
TARGET_LINK_LIBRARIES( detectFeatures 
    slambase
    ${OpenCV_LIBS} 
    ${PCL_LIBRARIES} 
    )

# 合并点云
ADD_EXECUTABLE( joinPointCloud joinPointCloud.cpp )
TARGET_LINK_LIBRARIES( joinPointCloud 
    slambase
    ${OpenCV_LIBS} 
    ${PCL_LIBRARIES} 
    )

# VO 视觉理程计
ADD_EXECUTABLE( visualOdometry visualOdometry.cpp )
TARGET_LINK_LIBRARIES( visualOdometry 
    slambase
    ${OpenCV_LIBS} 
    ${PCL_LIBRARIES} 
    )

# VO 视觉理程计
# ADD_EXECUTABLE( slamEnd slamEnd.cpp )
# TARGET_LINK_LIBRARIES( slamEnd 
#     slambase
#     ${OpenCV_LIBS} 
#     ${PCL_LIBRARIES} 
#     )
