QT += core testlib
CONFIG += c++17 console testcase
TEMPLATE = app
TARGET = tst_sqlsplitter
INCLUDEPATH += src

SOURCES += \
    tests/tst_sqlsplitter.cpp \
    src/sql/SqlSplitter.cpp

HEADERS += \
    src/sql/SqlSplitter.h
