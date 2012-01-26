TARGET = fb-qml

# Add more folders to ship with the application, here
folder_01.source = qml/re
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# Speed up launching on MeeGo/Harmattan when using applauncherd daemon
CONFIG += qdeclarative-boostable

QMAKE_CXXFLAGS += -std=c++0x

SOURCES += \
    main.cpp \
    resource.cpp \
    cutscenewidget.cpp

HEADERS += \
    data.h \
    resource.h \
    cutscenewidget.h

# Please do not modify the following two lines. Required for deployment.
include(qmlapplicationviewer/qmlapplicationviewer.pri)
qtcAddDeployment()