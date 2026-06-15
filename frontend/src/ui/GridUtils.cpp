#include "ui/GridUtils.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QSet>

void GridUtils::extract(const QTableWidget *grid, bool visibleOnly, bool selectedOnly,
                        QStringList &headersOut, QList<QStringList> &rowsOut) {
    headersOut.clear();
    rowsOut.clear();
    if (!grid) return;
    const int cols = grid->columnCount();
    for (int j = 0; j < cols; ++j) {
        auto *h = grid->horizontalHeaderItem(j);
        headersOut << (h ? h->text() : QString::number(j));
    }

    QSet<int> selRows;
    if (selectedOnly) {
        const auto ranges = grid->selectedRanges();
        for (const auto &r : ranges)
            for (int row = r.topRow(); row <= r.bottomRow(); ++row)
                selRows.insert(row);
    }

    for (int i = 0; i < grid->rowCount(); ++i) {
        if (visibleOnly && grid->isRowHidden(i)) continue;
        if (selectedOnly && !selRows.isEmpty() && !selRows.contains(i)) continue;
        QStringList row;
        for (int j = 0; j < cols; ++j) {
            auto *it = grid->item(i, j);
            row << (it ? it->text() : QString());
        }
        rowsOut << row;
    }
}
