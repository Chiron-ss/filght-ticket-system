cmake_minimum_required(VERSION 3.16)

project(FlightTicketClient VERSION 0.1 LANGUAGES CXX)

# Qt自动处理设置
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# C++标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Qt
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql LinguistTools)
set(QT_VERSION_MAJOR 6)

# 翻译文件
set(TS_FILES FlightTicketClient_zh_CN.ts)

# 主程序源文件
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    adminmainwindow.cpp
    adminmainwindow.h
    adminmainwindow.ui
    resources.qrc
    ${TS_FILES}
)

# 设置服务器文件路径（使用相对路径）
set(SERVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../server/FlightTicketServer")
set(SERVER_CPP_PATH "${SERVER_DIR}/DatabaseManager.cpp")
set(SERVER_H_PATH "${SERVER_DIR}/DatabaseManager.h")

# 检查文件是否存在
if(EXISTS "${SERVER_CPP_PATH}" AND EXISTS "${SERVER_H_PATH}")
    message(STATUS "找到服务器数据库文件: ${SERVER_CPP_PATH}")

    # 将服务器文件添加到项目中
    list(APPEND PROJECT_SOURCES
        "${SERVER_CPP_PATH}"
        "${SERVER_H_PATH}"
    )

    # 设置包含目录
    set(INCLUDE_SERVER_DIR TRUE)
else()
    message(WARNING "未找到服务器数据库文件，仅编译客户端")
    set(INCLUDE_SERVER_DIR FALSE)
endif()


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    # Qt 6
    qt_add_executable(FlightTicketClient
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        DatabaseManager.h
        DatabaseManager.cpp
    )
else()
    # Qt 5
    if(ANDROID)
        add_library(FlightTicketClient SHARED ${PROJECT_SOURCES})
    else()
        add_executable(FlightTicketClient ${PROJECT_SOURCES})
    endif()
endif()

# 添加包含目录
target_include_directories(FlightTicketClient PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
if(INCLUDE_SERVER_DIR)
    target_include_directories(FlightTicketClient PRIVATE "${SERVER_DIR}")
endif()

# 链接Qt库
target_link_libraries(FlightTicketClient PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
)

# 处理翻译文件
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR} ${TS_FILES})
else()
    qt5_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR} ${TS_FILES})
endif()

# 添加翻译文件到项目
target_sources(FlightTicketClient PRIVATE ${QM_FILES})

# 平台特定设置
if(Qt6_VERSION VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.FlightTicketClient)
endif()

set_target_properties(FlightTicketClient PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Qt 6最终化
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(FlightTicketClient)
endif()

# 输出配置信息
message(STATUS "========================================")
message(STATUS "项目配置完成")
message(STATUS "客户端路径: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "服务器路径: ${SERVER_DIR}")
message(STATUS "包含数据库文件: ${INCLUDE_SERVER_DIR}")
message(STATUS "========================================")
