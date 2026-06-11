QT += core
QT -= gui
CONFIG += c++17 console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = tst_persist
INCLUDEPATH += src

SOURCES += \
    tests/tst_persist.cpp \
    src/backend/ConnData.cpp \
    src/store/ConnectionStore.cpp

HEADERS += \
    src/backend/ConnData.h \
    src/backend/FuncId.h \
    src/store/ConnectionStore.h
