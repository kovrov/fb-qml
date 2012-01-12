QT += core gui

TARGET = dpoly
TEMPLATE = app

SOURCES += main.cpp\
        polywidget.cpp

HEADERS  += polywidget.h \
    data.h

QMAKE_CXXFLAGS += -std=c++0x
