QT += core gui

TARGET = dpoly
TEMPLATE = app

SOURCES += main.cpp\
        polywidget.cpp \
    resource.cpp

HEADERS  += polywidget.h \
    data.h \
    resource.h

QMAKE_CXXFLAGS += -std=c++0x
