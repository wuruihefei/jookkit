#ifndef JOOKKIT_GRIDUTILS_H
#define JOOKKIT_GRIDUTILS_H

#include <QStringList>
#include <QList>

class QTableWidget;

namespace GridUtils {
    // Extract headers and visible/selected row data from a QTableWidget.
    // visibleOnly: skip rows hidden by setRowHidden (from filter)
    // selectedOnly: if true AND there are selected ranges, only extract selected rows;
    //               if no selection, falls back to all visible rows
    void extract(const QTableWidget *grid, bool visibleOnly, bool selectedOnly,
                 QStringList &headersOut, QList<QStringList> &rowsOut);
}

#endif
