QT += core gui widgets network
CONFIG += c++17 console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = tst_persist2
INCLUDEPATH += src

SOURCES += \
    tests/tst_persist2.cpp \
    src/ui/ObjectTree.cpp \
    src/ui/Icons.cpp \
    src/backend/ConnData.cpp \
    src/backend/BackendProcess.cpp \
    src/backend/BackendClient.cpp \
    src/store/ConnectionStore.cpp

HEADERS += \
    src/ui/ObjectTree.h \
    src/ui/Icons.h \
    src/backend/ConnData.h \
    src/backend/BackendProcess.h \
    src/backend/BackendClient.h \
    src/backend/FuncId.h \
    src/store/ConnectionStore.h
