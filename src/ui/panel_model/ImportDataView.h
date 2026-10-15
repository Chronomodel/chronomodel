/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#ifndef IMPORTDATAVIEW_H
#define IMPORTDATAVIEW_H

#include <QDialog>
#include <QTableWidget>

class QTableWidgetItem;

class Button;
class ImportDataView;
class HelpWidget;


class ImportDataTable : public QTableWidget
{
    Q_OBJECT
public:
    ImportDataTable(ImportDataView* importView, QWidget* parent = nullptr);
    virtual ~ImportDataTable();

private slots:
    void updateTableHeaders();

protected:
    QMimeData* mimeData(const QList<QTableWidgetItem*> &items) const;

private:
    ImportDataView* mImportView;
};



class ImportDataView: public QWidget
{
    Q_OBJECT
public:
    ImportDataView(QWidget* parent = nullptr, const bool show_help = false, Qt::WindowFlags flags =  Qt::Widget);
    ~ImportDataView();

public slots:
    void removeCsvRows(QList<int> rows);
    void errorCsvRows(QList<int> rows);
    void setHelpVisible(const bool visible);

private slots:
    void browse();
    void exportDates();

protected:
    void paintEvent(QPaintEvent*);
   // void resizeEvent(QResizeEvent*);

private:
    Button* mBrowseBut;
    Button* mExportBut;
    ImportDataTable* mTable;
    HelpWidget* mHelp;

public:
    QString mPath;
};

#endif
