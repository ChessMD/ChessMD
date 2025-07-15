cmake_minimum_required(VERSION 3.16)

project(ChessMD VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets QuickWidgets Svg Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets QuickWidgets Svg Sql)

set(PROJECT_SOURCES
        main.cpp




)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ChessMD
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}



        databaseviewer.h databaseviewer.cpp databaseviewer.ui
        styles/tablestyle.qss
        tabledelegate.h tabledelegate.cpp
        pgnuploader.h pgnuploader.cpp pgnuploader.ui

        databaselibrary.h databaselibrary.cpp databaselibrary.ui
        databaseuploader.h databaseuploader.cpp databaseuploader.ui
        pgngamedata.h pgngamedata.cpp
        streamparser.h streamparser.cpp
        databaseviewermodel.h databaseviewermodel.cpp
        helpers.h
        helpers.cpp
        chesstabhost.h chesstabhost.cpp
        databasefilter.h databasefilter.cpp databasefilter.ui
        databasefilterproxymodel.h databasefilterproxymodel.cpp
        chessgamewindow.h chessgamewindow.cpp
        qml.qrc
        img/piece/alpha/bB.svg img/piece/alpha/bK.svg img/piece/alpha/bN.svg img/piece/alpha/bP.svg img/piece/alpha/bQ.svg img/piece/alpha/bR.svg img/piece/alpha/wB.svg img/piece/alpha/wK.svg img/piece/alpha/wN.svg img/piece/alpha/wP.svg img/piece/alpha/wQ.svg img/piece/alpha/wR.svg img/piece/anarcandy/bB.svg img/piece/anarcandy/bK.svg img/piece/anarcandy/bN.svg img/piece/anarcandy/bP.svg img/piece/anarcandy/bQ.svg img/piece/anarcandy/bR.svg img/piece/anarcandy/wB.svg img/piece/anarcandy/wK.svg img/piece/anarcandy/wN.svg img/piece/anarcandy/wP.svg img/piece/anarcandy/wQ.svg img/piece/anarcandy/wR.svg img/piece/caliente/bB.svg img/piece/caliente/bK.svg img/piece/caliente/bN.svg img/piece/caliente/bP.svg img/piece/caliente/bQ.svg img/piece/caliente/bR.svg img/piece/caliente/wB.svg img/piece/caliente/wK.svg img/piece/caliente/wN.svg img/piece/caliente/wP.svg img/piece/caliente/wQ.svg img/piece/caliente/wR.svg img/piece/california/bB.svg img/piece/california/bK.svg img/piece/california/bN.svg img/piece/california/bP.svg img/piece/california/bQ.svg img/piece/california/bR.svg img/piece/california/wB.svg img/piece/california/wK.svg img/piece/california/wN.svg img/piece/california/wP.svg img/piece/california/wQ.svg img/piece/california/wR.svg img/piece/cardinal/bB.svg img/piece/cardinal/bK.svg img/piece/cardinal/bN.svg img/piece/cardinal/bP.svg img/piece/cardinal/bQ.svg img/piece/cardinal/bR.svg img/piece/cardinal/wB.svg img/piece/cardinal/wK.svg img/piece/cardinal/wN.svg img/piece/cardinal/wP.svg img/piece/cardinal/wQ.svg img/piece/cardinal/wR.svg img/piece/cburnett/bB.svg img/piece/cburnett/bK.svg img/piece/cburnett/bN.svg img/piece/cburnett/bP.svg img/piece/cburnett/bQ.svg img/piece/cburnett/bR.svg img/piece/cburnett/wB.svg img/piece/cburnett/wK.svg img/piece/cburnett/wN.svg img/piece/cburnett/wP.svg img/piece/cburnett/wQ.svg img/piece/cburnett/wR.svg img/piece/celtic/bB.svg img/piece/celtic/bK.svg img/piece/celtic/bN.svg img/piece/celtic/bP.svg img/piece/celtic/bQ.svg img/piece/celtic/bR.svg img/piece/celtic/wB.svg img/piece/celtic/wK.svg img/piece/celtic/wN.svg img/piece/celtic/wP.svg img/piece/celtic/wQ.svg img/piece/celtic/wR.svg img/piece/chess7/bB.svg img/piece/chess7/bK.svg img/piece/chess7/bN.svg img/piece/chess7/bP.svg img/piece/chess7/bQ.svg img/piece/chess7/bR.svg img/piece/chess7/wB.svg img/piece/chess7/wK.svg img/piece/chess7/wN.svg img/piece/chess7/wP.svg img/piece/chess7/wQ.svg img/piece/chess7/wR.svg img/piece/chessnut/bB.svg img/piece/chessnut/bK.svg img/piece/chessnut/bN.svg img/piece/chessnut/bP.svg img/piece/chessnut/bQ.svg img/piece/chessnut/bR.svg img/piece/chessnut/wB.svg img/piece/chessnut/wK.svg img/piece/chessnut/wN.svg img/piece/chessnut/wP.svg img/piece/chessnut/wQ.svg img/piece/chessnut/wR.svg img/piece/companion/bB.svg img/piece/companion/bK.svg img/piece/companion/bN.svg img/piece/companion/bP.svg img/piece/companion/bQ.svg img/piece/companion/bR.svg img/piece/companion/wB.svg img/piece/companion/wK.svg img/piece/companion/wN.svg img/piece/companion/wP.svg img/piece/companion/wQ.svg img/piece/companion/wR.svg img/piece/cooke/bB.svg img/piece/cooke/bK.svg img/piece/cooke/bN.svg img/piece/cooke/bP.svg img/piece/cooke/bQ.svg img/piece/cooke/bR.svg img/piece/cooke/wB.svg img/piece/cooke/wK.svg img/piece/cooke/wN.svg img/piece/cooke/wP.svg img/piece/cooke/wQ.svg img/piece/cooke/wR.svg img/piece/disguised/b.svg img/piece/disguised/bB.svg img/piece/disguised/bK.svg img/piece/disguised/bN.svg img/piece/disguised/bP.svg img/piece/disguised/bQ.svg img/piece/disguised/bR.svg img/piece/disguised/w.svg img/piece/disguised/wB.svg img/piece/disguised/wK.svg img/piece/disguised/wN.svg img/piece/disguised/wP.svg img/piece/disguised/wQ.svg img/piece/disguised/wR.svg img/piece/dubrovny/bB.svg img/piece/dubrovny/bK.svg img/piece/dubrovny/bN.svg img/piece/dubrovny/bP.svg img/piece/dubrovny/bQ.svg img/piece/dubrovny/bR.svg img/piece/dubrovny/wB.svg img/piece/dubrovny/wK.svg img/piece/dubrovny/wN.svg img/piece/dubrovny/wP.svg img/piece/dubrovny/wQ.svg img/piece/dubrovny/wR.svg img/piece/fantasy/bB.svg img/piece/fantasy/bK.svg img/piece/fantasy/bN.svg img/piece/fantasy/bP.svg img/piece/fantasy/bQ.svg img/piece/fantasy/bR.svg img/piece/fantasy/wB.svg img/piece/fantasy/wK.svg img/piece/fantasy/wN.svg img/piece/fantasy/wP.svg img/piece/fantasy/wQ.svg img/piece/fantasy/wR.svg img/piece/fresca/bB.svg img/piece/fresca/bK.svg img/piece/fresca/bN.svg img/piece/fresca/bP.svg img/piece/fresca/bQ.svg img/piece/fresca/bR.svg img/piece/fresca/wB.svg img/piece/fresca/wK.svg img/piece/fresca/wN.svg img/piece/fresca/wP.svg img/piece/fresca/wQ.svg img/piece/fresca/wR.svg img/piece/gioco/bB.svg img/piece/gioco/bK.svg img/piece/gioco/bN.svg img/piece/gioco/bP.svg img/piece/gioco/bQ.svg img/piece/gioco/bR.svg img/piece/gioco/wB.svg img/piece/gioco/wK.svg img/piece/gioco/wN.svg img/piece/gioco/wP.svg img/piece/gioco/wQ.svg img/piece/gioco/wR.svg img/piece/governor/bB.svg img/piece/governor/bK.svg img/piece/governor/bN.svg img/piece/governor/bP.svg img/piece/governor/bQ.svg img/piece/governor/bR.svg img/piece/governor/wB.svg img/piece/governor/wK.svg img/piece/governor/wN.svg img/piece/governor/wP.svg img/piece/governor/wQ.svg img/piece/governor/wR.svg img/piece/horsey/bB.svg img/piece/horsey/bK.svg img/piece/horsey/bN.svg img/piece/horsey/bP.svg img/piece/horsey/bQ.svg img/piece/horsey/bR.svg img/piece/horsey/wB.svg img/piece/horsey/wK.svg img/piece/horsey/wN.svg img/piece/horsey/wP.svg img/piece/horsey/wQ.svg img/piece/horsey/wR.svg img/piece/icpieces/bB.svg img/piece/icpieces/bK.svg img/piece/icpieces/bN.svg img/piece/icpieces/bP.svg img/piece/icpieces/bQ.svg img/piece/icpieces/bR.svg img/piece/icpieces/wB.svg img/piece/icpieces/wK.svg img/piece/icpieces/wN.svg img/piece/icpieces/wP.svg img/piece/icpieces/wQ.svg img/piece/icpieces/wR.svg img/piece/kiwen-suwi/bB.svg img/piece/kiwen-suwi/bK.svg img/piece/kiwen-suwi/bN.svg img/piece/kiwen-suwi/bP.svg img/piece/kiwen-suwi/bQ.svg img/piece/kiwen-suwi/bR.svg img/piece/kiwen-suwi/wB.svg img/piece/kiwen-suwi/wK.svg img/piece/kiwen-suwi/wN.svg img/piece/kiwen-suwi/wP.svg img/piece/kiwen-suwi/wQ.svg img/piece/kiwen-suwi/wR.svg img/piece/kosal/bB.svg img/piece/kosal/bK.svg img/piece/kosal/bN.svg img/piece/kosal/bP.svg img/piece/kosal/bQ.svg img/piece/kosal/bR.svg img/piece/kosal/wB.svg img/piece/kosal/wK.svg img/piece/kosal/wN.svg img/piece/kosal/wP.svg img/piece/kosal/wQ.svg img/piece/kosal/wR.svg img/piece/leipzig/bB.svg img/piece/leipzig/bK.svg img/piece/leipzig/bN.svg img/piece/leipzig/bP.svg img/piece/leipzig/bQ.svg img/piece/leipzig/bR.svg img/piece/leipzig/wB.svg img/piece/leipzig/wK.svg img/piece/leipzig/wN.svg img/piece/leipzig/wP.svg img/piece/leipzig/wQ.svg img/piece/leipzig/wR.svg img/piece/letter/bB.svg img/piece/letter/bK.svg img/piece/letter/bN.svg img/piece/letter/bP.svg img/piece/letter/bQ.svg img/piece/letter/bR.svg img/piece/letter/wB.svg img/piece/letter/wK.svg img/piece/letter/wN.svg img/piece/letter/wP.svg img/piece/letter/wQ.svg img/piece/letter/wR.svg img/piece/maestro/bB.svg img/piece/maestro/bK.svg img/piece/maestro/bN.svg img/piece/maestro/bP.svg img/piece/maestro/bQ.svg img/piece/maestro/bR.svg img/piece/maestro/wB.svg img/piece/maestro/wK.svg img/piece/maestro/wN.svg img/piece/maestro/wP.svg img/piece/maestro/wQ.svg img/piece/maestro/wR.svg img/piece/merida/bB.svg img/piece/merida/bK.svg img/piece/merida/bN.svg img/piece/merida/bP.svg img/piece/merida/bQ.svg img/piece/merida/bR.svg img/piece/merida/wB.svg img/piece/merida/wK.svg img/piece/merida/wN.svg img/piece/merida/wP.svg img/piece/merida/wQ.svg img/piece/merida/wR.svg img/piece/monarchy/bB.svg img/piece/monarchy/bK.svg img/piece/monarchy/bN.svg img/piece/monarchy/bP.svg img/piece/monarchy/bQ.svg img/piece/monarchy/bR.svg img/piece/monarchy/wB.svg img/piece/monarchy/wK.svg img/piece/monarchy/wN.svg img/piece/monarchy/wP.svg img/piece/monarchy/wQ.svg img/piece/monarchy/wR.svg img/piece/mono/bB.svg img/piece/mono/bK.svg img/piece/mono/bN.svg img/piece/mono/bP.svg img/piece/mono/bQ.svg img/piece/mono/bR.svg img/piece/mono/wB.svg img/piece/mono/wK.svg img/piece/mono/wN.svg img/piece/mono/wP.svg img/piece/mono/wQ.svg img/piece/mono/wR.svg img/piece/mpchess/bB.svg img/piece/mpchess/bK.svg img/piece/mpchess/bN.svg img/piece/mpchess/bP.svg img/piece/mpchess/bQ.svg img/piece/mpchess/bR.svg img/piece/mpchess/wB.svg img/piece/mpchess/wK.svg img/piece/mpchess/wN.svg img/piece/mpchess/wP.svg img/piece/mpchess/wQ.svg img/piece/mpchess/wR.svg img/piece/pirouetti/bB.svg img/piece/pirouetti/bK.svg img/piece/pirouetti/bN.svg img/piece/pirouetti/bP.svg img/piece/pirouetti/bQ.svg img/piece/pirouetti/bR.svg img/piece/pirouetti/wB.svg img/piece/pirouetti/wK.svg img/piece/pirouetti/wN.svg img/piece/pirouetti/wP.svg img/piece/pirouetti/wQ.svg img/piece/pirouetti/wR.svg img/piece/pixel/bB.svg img/piece/pixel/bK.svg img/piece/pixel/bN.svg img/piece/pixel/bP.svg img/piece/pixel/bQ.svg img/piece/pixel/bR.svg img/piece/pixel/wB.svg img/piece/pixel/wK.svg img/piece/pixel/wN.svg img/piece/pixel/wP.svg img/piece/pixel/wQ.svg img/piece/pixel/wR.svg img/piece/reillycraig/bB.svg img/piece/reillycraig/bK.svg img/piece/reillycraig/bN.svg img/piece/reillycraig/bP.svg img/piece/reillycraig/bQ.svg img/piece/reillycraig/bR.svg img/piece/reillycraig/wB.svg img/piece/reillycraig/wK.svg img/piece/reillycraig/wN.svg img/piece/reillycraig/wP.svg img/piece/reillycraig/wQ.svg img/piece/reillycraig/wR.svg img/piece/rhosgfx/bB.svg img/piece/rhosgfx/bK.svg img/piece/rhosgfx/bN.svg img/piece/rhosgfx/bP.svg img/piece/rhosgfx/bQ.svg img/piece/rhosgfx/bR.svg img/piece/rhosgfx/wB.svg img/piece/rhosgfx/wK.svg img/piece/rhosgfx/wN.svg img/piece/rhosgfx/wP.svg img/piece/rhosgfx/wQ.svg img/piece/rhosgfx/wR.svg img/piece/riohacha/bB.svg img/piece/riohacha/bK.svg img/piece/riohacha/bN.svg img/piece/riohacha/bP.svg img/piece/riohacha/bQ.svg img/piece/riohacha/bR.svg img/piece/riohacha/wB.svg img/piece/riohacha/wK.svg img/piece/riohacha/wN.svg img/piece/riohacha/wP.svg img/piece/riohacha/wQ.svg img/piece/riohacha/wR.svg img/piece/shapes/bB.svg img/piece/shapes/bK.svg img/piece/shapes/bN.svg img/piece/shapes/bP.svg img/piece/shapes/bQ.svg img/piece/shapes/bR.svg img/piece/shapes/wB.svg img/piece/shapes/wK.svg img/piece/shapes/wN.svg img/piece/shapes/wP.svg img/piece/shapes/wQ.svg img/piece/shapes/wR.svg img/piece/spatial/bB.svg img/piece/spatial/bK.svg img/piece/spatial/bN.svg img/piece/spatial/bP.svg img/piece/spatial/bQ.svg img/piece/spatial/bR.svg img/piece/spatial/wB.svg img/piece/spatial/wK.svg img/piece/spatial/wN.svg img/piece/spatial/wP.svg img/piece/spatial/wQ.svg img/piece/spatial/wR.svg img/piece/staunty/bB.svg img/piece/staunty/bK.svg img/piece/staunty/bN.svg img/piece/staunty/bP.svg img/piece/staunty/bQ.svg img/piece/staunty/bR.svg img/piece/staunty/wB.svg img/piece/staunty/wK.svg img/piece/staunty/wN.svg img/piece/staunty/wP.svg img/piece/staunty/wQ.svg img/piece/staunty/wR.svg img/piece/tatiana/bB.svg img/piece/tatiana/bK.svg img/piece/tatiana/bN.svg img/piece/tatiana/bP.svg img/piece/tatiana/bQ.svg img/piece/tatiana/bR.svg img/piece/tatiana/wB.svg img/piece/tatiana/wK.svg img/piece/tatiana/wN.svg img/piece/tatiana/wP.svg img/piece/tatiana/wQ.svg img/piece/tatiana/wR.svg img/piece/xkcd/bB.svg img/piece/xkcd/bK.svg img/piece/xkcd/bN.svg img/piece/xkcd/bP.svg img/piece/xkcd/bQ.svg img/piece/xkcd/bR.svg img/piece/xkcd/wB.svg img/piece/xkcd/wK.svg img/piece/xkcd/wN.svg img/piece/xkcd/wP.svg img/piece/xkcd/wQ.svg img/piece/xkcd/wR.svg
        notation.h notation.cpp
        notationviewer.h notationviewer.cpp
        variationdialogue.h variationdialogue.cpp
        chessposition.h chessposition.cpp
        customtitlebar.ui
        img/close.png img/fileicon.png img/fileuploadicon.png img/maxedmaximize.png img/maximize.png img/minimize.png
        resource.qrc
        uciengine.h uciengine.cpp
        enginewidget.h enginewidget.cpp
        mainwindow.h mainwindow.cpp
        chessqsettings.h chessqsettings.cpp
        chessgametabdialog.h
        chessgametabdialog.cpp
        img/addfile.png
        chessgamefilesdata.h
        chessgamefilesdata.cpp
        chessgamefilesdata.cpp
        README.md
        user-guide.md
        screenshots/Figure_2.1.1a.png screenshots/Figure_2.1a.png screenshots/Figure_2.1b.png screenshots/Figure_2.1c.png screenshots/Figure_2.2.3a.png screenshots/Figure_2.2.4a.png screenshots/Figure_2.3.3a.png screenshots/Figure_2.3.7a.png
        enginelinewidget.h enginelinewidget.cpp
        pgnsavedialog.h pgnsavedialog.cpp



    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ChessMD APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(ChessMD SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(ChessMD
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(ChessMD PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt6::QuickWidgets Qt6::Svg Qt6::Sql)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ChessMD)
endif()
set_target_properties(ChessMD PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ChessMD
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ChessMD)
endif()
