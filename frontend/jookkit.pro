QT += core gui widgets network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17
TARGET = jookkit
TEMPLATE = app

INCLUDEPATH += src

win32: RC_ICONS = resources/jookkit.ico

RESOURCES += resources.qrc

SOURCES += \
    src/main.cpp \
    src/ui/MainWindow.cpp \
    src/ui/HistoryPane.cpp \
    src/ui/ContentWidget.cpp \
    src/ui/ObjectTree.cpp \
    src/ui/ConnDialog.cpp \
    src/ui/QueryForm.cpp \
    src/ui/GridUtils.cpp \
    src/ui/TableStructureForm.cpp \
    src/ui/TableDataForm.cpp \
    src/ui/InformationPane.cpp \
    src/ui/Icons.cpp \
    src/ui/OptionsDialog.cpp \
    src/ui/FindReplaceDialog.cpp \
    src/ui/UserManagerDialog.cpp \
    src/ui/SqlEditor.cpp \
    src/sql/SqlSplitter.cpp \
    src/sql/SqlHighlighter.cpp \
    src/sql/SqlFormat.cpp \
    src/store/ConnectionStore.cpp \
    src/store/FavoriteStore.cpp \
    src/store/HistoryStore.cpp \
    src/export/Exporter.cpp \
    src/import/CsvReader.cpp \
    src/import/JsonReader.cpp \
    src/import/ColumnMapping.cpp \
    src/import/ImportRunner.cpp \
    src/backend/ConnData.cpp \
    src/backend/BackendProcess.cpp \
    src/backend/BackendClient.cpp

HEADERS += \
    src/ui/MainWindow.h \
    src/ui/HistoryPane.h \
    src/ui/ContentWidget.h \
    src/ui/ObjectTree.h \
    src/ui/ConnDialog.h \
    src/ui/QueryForm.h \
    src/ui/GridUtils.h \
    src/ui/TableStructureForm.h \
    src/ui/TableDataForm.h \
    src/ui/InformationPane.h \
    src/ui/Icons.h \
    src/ui/OptionsDialog.h \
    src/ui/FindReplaceDialog.h \
    src/ui/UserManagerDialog.h \
    src/ui/SqlEditor.h \
    src/sql/SqlSplitter.h \
    src/sql/SqlHighlighter.h \
    src/sql/SqlFormat.h \
    src/store/ConnectionStore.h \
    src/store/FavoriteStore.h \
    src/store/HistoryStore.h \
    src/export/Exporter.h \
    src/import/ParseResult.h \
    src/import/CsvReader.h \
    src/import/JsonReader.h \
    src/import/ColumnMapping.h \
    src/import/ImportRunner.h \
    src/backend/ConnData.h \
    src/backend/BackendProcess.h \
    src/backend/BackendClient.h \
    src/backend/FuncId.h
