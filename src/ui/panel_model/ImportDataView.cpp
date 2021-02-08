/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "ImportDataView.h"
#include "PluginManager.h"
#include "../PluginAbstract.h"
#include "QtUtilities.h"
#include "HelpWidget.h"
#include "Button.h"
#include "Label.h"
#include "MainWindow.h"
#include "Project.h"

#include <QtWidgets>


ImportDataView::ImportDataView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    mBrowseBut = new Button(tr("Load CSV file..."), this);
    mExportBut = new Button(tr("Export all project data as CSV"), this);
    mHelp = new HelpWidget(this);
    mHelp->setLink("https://chronomodel.com/storage/medias/3_chronomodel_user_manual.pdf#page=31"); //chapter 3.4.2.1 Radiocarbon dating (14C)

    mHelp->setText(tr("Your CSV file must contain 1 data per row. Each row must start with an Event name, the second row is the datation method to use. Allowed datation methods are : 14C, AM, Gauss, Unif, TL/OSL.\nComments are allowed in your CSV. They must start with  # or // and can be placed at the end of a data row. When placed at the begining of a row, the whole row is ignored.\r Be careful, cell separator and decimal separator of the CSV file should be those defined in the Application Settings, otherwise the CSV file will not be opened"));

    mTable = new ImportDataTable(this, this);
    mTable->setAlternatingRowColors(true);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTable->setDragEnabled(true);
    mTable->setDragDropMode(QAbstractItemView::DragOnly);

    connect(mBrowseBut, &Button::pressed, this, &ImportDataView::browse);
    connect(mExportBut, &Button::pressed, this,  &ImportDataView::exportDates);
}

ImportDataView::~ImportDataView()
{

}

/**
 * @brief Import data from a CSV file in the table
 * @todo File encoding must be UTF8, Unix LF !!
 * Title; toy csv file
 *  Structure; Terrestrial
 *  // just comment
 *  14C;onshore;1200;30;intcal13.14c;
 *  14C;WithWiggle;50000;5;intcal13.14c;0;0;range;5;10
 *  Structure; Event : Oceanic
 *  14C;shell;2900;36;marine04.14c;-150;20
 *  14C;oyster;3000;30;marine13.14c;200;10
 *
 * Since version 1.6.4_alpha
 * example of csv file below
 *
// this is a comment :test file for csv with Event's name
Title; Toy
EVENT1;AM;mesure_inclinaison_a_Paris;inclination;68,5;0;0;0,98;gal2002sph2014_i.ref
EventAM;AM;mesure_Declinaison_a_Paris;declination;60;10;0;3;gal2002sph2014_d.ref
Event14C;14C;14C1;1225;30;intcal13.14c;0;0
// No event name
;Gauss;Gauss_ss_event;0;50;equation;0;1;0;none
// event with data name missing
Event9;Gauss;;650;50;none;none
Event1;Gauss; New Data;24;50;none;none
Event1;14C;New 14C;1600;35;intcal13.14c;0;0;none
Event1;14C;With Wiggle;50000;5;intcal13.14c;0;0;range;5;10
# test info
Structure; Terrestrial
Event8;Gauss; New Data;-79;50;none;none

EventAM;AM;incli;inclination;60;0;0;1;gal2002sph2014_i.ref;none
Event14C;14C;shell;2900;36;marine04.14c;-150;20;none
Event1;14C;onshore;1600;35;intcal13.14c;0;0;none
 *
 */
void ImportDataView::browse()
{
    const QString currentDir = MainWindow::getInstance()->getCurrentPath();
    const QString path = QFileDialog::getOpenFileName(qApp->activeWindow(), tr("Open CSV File"), currentDir, "CSV File (*.csv)");

    if (!path.isEmpty()) {
        QFileInfo info(path);
        mPath = info.absolutePath();
        MainWindow::getInstance()->setCurrentPath(mPath);

        while (mTable->rowCount() > 0)
            mTable->removeRow(0);

        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            QList<QStringList> data;

           /**
            * @todo File encoding must be UTF8, Unix LF !!
            */
            //QTextCodec* codec = stream.codec();

            int rows (0);
            int cols (0);

            QStringList headers;
            const QStringList pluginNames = PluginManager::getPluginsNames();

            const QString csvSep = AppSettings::mCSVCellSeparator;

            // endline detection, we want to find which system, OsX, Mac, Windows made this file
           // QString line = stream.readLine();
           /* char * pText;

            stream.reset();

             //FILE * pFile ;

            char delim ;
            char* c;
            do {
                  stream.device()->getChar(c);
                  if(*c == 0x0A) {// LF (Line feed, '\n', 0x0A, 10 in decimal) - Linux, OS X
                      delim = 0x0A;
                      qDebug()<<"ENDLINE Delim LF Line feed";
                  }
                  if(*c == 0x0A) { //CR followed by LF (CR+LF, '\r\n', 0x0D0A) - Microsoft Windows
                      delim = 0x0D0A;
                      qDebug()<<"ENDLINE Delim CR+LF CR followed by LF";
                  }
                  if(*c == 0x0A) {// CR (Carriage return, '\r', 0x0D, 13 in decimal) - Mac OS up to version 9, and OS-9
                      delim = 0x0D;
                      qDebug()<<"ENDLINE Delim CR Carriage return";
                  }
             } while (*c != EOF);
           // fclose (pFile);


             * if(line.contains("\n")) {// LF (Line feed, '\n', 0x0A, 10 in decimal) - Linux, OS X
                delim = 0x0A;
                qDebug()<<"ENDLINE Delim LF Line feed";
            }
            if(line.contains("\r\n")) { //CR followed by LF (CR+LF, '\r\n', 0x0D0A) - Microsoft Windows
                delim = 0x0D0A;
                qDebug()<<"ENDLINE Delim CR+LF CR followed by LF";
            }
            if(line.contains("\r")) {// CR (Carriage return, '\r', 0x0D, 13 in decimal) - Mac OS up to version 9, and OS-9
                delim = 0x0D;
                qDebug()<<"ENDLINE Delim CR Carriage return";
            }
            std::string stdLine;
            std::ifstream input(file.fileName().toStdString());

            //http://en.cppreference.com/w/cpp/string/basic_string/getline
            while(std::getline(input, stdLine, delim))

            */

            // Read every lines of the file
            int noNameCount (1);
            while (!stream.atEnd()) {
                const QString line = stream.readLine();
                QStringList values = line.split(csvSep);
                if (values.size() > 0) {
                    //qDebug()<<"ImportDataView::browse() "<<values.at(0).toUpper();
                    if (values.size() > 2 && values.at(0) == "") {
                        values[0]="No Name "+ QString::number(noNameCount);
                        ++noNameCount;
                    }
                    if (isComment(values.at(0))) {
                        continue;

                    } else if (values.at(0).contains("title", Qt::CaseInsensitive) && !values.at(0).contains("ntitle", Qt::CaseInsensitive)) {
                        headers << "TITLE";

                        QStringList titleText;
                        values.push_front("");
                        foreach (const QString val, values) {
                            if (val.toUpper() != "TITLE")
                                titleText.append(val);
                        }
                        data << titleText;
                        cols = (values.size() > cols) ? values.size() : cols;
                        ++rows;

                    } else if (values.at(0).contains("structure", Qt::CaseInsensitive)) {
                        headers << "STRUCTURE";
                        QStringList titleText;
                        values.push_front("");
                        foreach (const QString val, values) {
                            if (val.toUpper() != "STRUCTURE")
                                titleText.append(val);
                        }

                        data << titleText;
                        cols = (values.size() > cols) ? values.size() : cols;
                        ++rows;

                    } else if (values.size() > 2) {
                        // Display the line only if we have a plugin to import it !
                        const QString EventName = values.at(0);
                        const QString pluginName = values.at(1);
                        if (pluginNames.contains(pluginName, Qt::CaseInsensitive)) {
                            headers << EventName;
                            data << values;

                            // Adapt max columns count if necessary
                            cols = (values.size() > cols) ? values.size() : cols;
                            ++rows;

                        } else if (pluginName.contains("bound", Qt::CaseInsensitive)) {
                            headers << values.at(0);
                            data << values;

                            // Adapt max columns count if necessary
                            cols = (values.size() > cols) ? values.size() : cols;
                            ++rows;
                        }
                    }
                } else {
                    file.close();
                    return;
                }

            }

            file.close();
            mTable->setRowCount(rows);
            mTable->setColumnCount(cols);
            mTable->setVerticalHeaderLabels(headers);

            /*
             * Update table view with data constructed before
             */
            if (data.isEmpty()){
                // to have a \ char in string, in C++ you must use two char
                QMessageBox message(QMessageBox::Warning, tr("Bad file"), tr("Maybe you need to check the manual to build your CSV file !") + " <a href='https://chronomodel.com/storage/medias/3_chronomodel_user_manual.pdf#page=29 '>"+ tr("More...")+"</a> ",
                                    QMessageBox::Ok, qApp->activeWindow());
                message.exec();
            } else {
                for (int i=0; i<data.size(); ++i) {
                    const QStringList d = data.at(i);

                    for (int j=1; j<d.size(); ++j) {
                        // Skip the first column containing the eventName (already used in the table line header)

                        if (j != 0) {
                            QTableWidgetItem* item = new QTableWidgetItem(d.at(j).simplified());
                            mTable->setItem(i, j-1, item);
                        }
                    }
                }

                mTable->setCurrentCell(0, 0);
         }
        }
    }
}

/**
 * @brief Export all data inside event, in a compatible csv file to import them later
 *  the bound are insert as Structure but they are not importable
 */
void ImportDataView::exportDates()
{
    QString currentDir = MainWindow::getInstance()->getCurrentPath();
    QString path = QFileDialog::getSaveFileName(qApp->activeWindow(), tr("Save as CSV"), currentDir, "CSV File (*.csv)");

    if (!path.isEmpty())
    {
        QFileInfo info(path);
        mPath = info.absolutePath();
        MainWindow::getInstance()->setCurrentPath(mPath);

        QString sep = AppSettings::mCSVCellSeparator;
        QLocale csvLocal = AppSettings::mCSVDecSeparator == "." ? QLocale::English : QLocale::French;
        csvLocal.setNumberOptions(QLocale::OmitGroupSeparator);

        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream stream(&file);

            Project* project = MainWindow::getInstance()->getProject();
            QJsonArray events = project->mState[STATE_EVENTS].toArray();

            stream << "Title" << sep << AppSettings::mLastFile << Qt::endl;
            bool isChronocurve = project->mState[STATE_CHRONOCURVE].toObject().value(STATE_CHRONOCURVE_ENABLED).toBool();
            // int chronocurveStartColumn = 15;

            for (int i=0; i<events.size(); ++i) {
                QJsonObject event = events[i].toObject();
                QJsonArray dates = event[STATE_EVENT_DATES].toArray();

                int type = event[STATE_EVENT_TYPE].toInt();
                const QString eventName = event[STATE_NAME].toString();
                if (type == Event::eKnown) {
                    stream << eventName << sep << "Bound" <<  sep << event.value(STATE_EVENT_KNOWN_FIXED).toDouble() << Qt::endl;

                } else {
                   for (int j=0; j<dates.size(); ++j) {
                        QJsonObject date = dates.at(j).toObject();
                        try{
                            Date d (date);
                            if (!d.isNull()) {
                                QStringList dateCsv = d.toCSV(csvLocal);
                                
                                if (isChronocurve) {
                                    // Chronocurve values start at column 15.
                                    // They must be put from column 14 in dateCsv,
                                    // because the row is shifted by one column at inserting eventName (see below)
                                    int chronocurveStartColumn = 15;
                                    while (dateCsv.count() < (chronocurveStartColumn-2)) {
                                        dateCsv.append("");
                                    }
                                    dateCsv.append(QString::number(event[STATE_EVENT_Y_INT].toDouble()));
                                    dateCsv.append(QString::number(event[STATE_EVENT_S_INT].toDouble()));
                                    dateCsv.append(QString::number(event[STATE_EVENT_Y_INC].toDouble()));
                                    dateCsv.append(QString::number(event[STATE_EVENT_Y_DEC].toDouble()));
                                    dateCsv.append(QString::number(event[STATE_EVENT_S_INC].toDouble()));
                                    dateCsv.append(QString::number(event[STATE_EVENT_Y_INT].toDouble()));
                                    dateCsv.append(QString::number(event[STATE_EVENT_S_INT].toDouble()));
                                }

                                stream << eventName << sep;
                                stream << dateCsv.join(sep) << Qt::endl;
                            }
                        }
                        catch (QString error) {
                            QMessageBox message(QMessageBox::Critical,
                                qApp->applicationName() + " " + qApp->applicationVersion(),
                                tr("Error : %1").arg(error),
                                QMessageBox::Ok,
                                qApp->activeWindow());

                            message.exec();
                        }
                    }
                }

            }
            file.close();
        }
    }
}

void ImportDataView::removeCsvRows(QList<int> rows)
{
    sortIntList(rows);
    for (int i = rows.size()-1; i >= 0; --i) {
        //qDebug() << "Removing row : " << rows[i];
        //mTable->removeRow(rows[i]);

        for (int c = 0; c < mTable->columnCount(); ++c) {
            QTableWidgetItem* item = mTable->item(rows.at(i), c);
            if (item)
                item->setBackground(QColor(100, 200, 100));

        }
    }
}

void ImportDataView::errorCsvRows(QList<int> rows)
{
    sortIntList(rows);
    for (int i = rows.size()-1; i >= 0; --i) {
        for (int c = 0; c < mTable->columnCount(); ++c) {
            QTableWidgetItem* item = mTable->item(rows.at(i), c);
            if (item)
                item->setBackground(QColor(220, 110, 94));
        }
    }
}

void ImportDataView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.fillRect(rect(), QColor(200, 200, 200));
}

void ImportDataView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);

    int m (5);
    int butH (25);
    int helpH = mHelp->heightForWidth(width() - 2*m);

    mBrowseBut->setGeometry(m, m, (width() - 3*m)/2, butH);
    mExportBut->setGeometry(2*m + (width() - 3*m)/2, m, (width() - 3*m)/2, butH);

    mTable->setGeometry(0, 2*m + butH, width(), height() - 4*m - butH - helpH);
    mHelp->setGeometry(m, height() - helpH - m, width() - 2*m, helpH);
}

// ------------------------------------------------------------------------------------

// Table

ImportDataTable::ImportDataTable(ImportDataView* importView, QWidget* parent):QTableWidget(parent),
mImportView(importView)
{
    connect(this, &ImportDataTable::itemSelectionChanged, this, &ImportDataTable::updateTableHeaders);
}

ImportDataTable::~ImportDataTable()
{

}

/**
 * @brief ImportDataTable::mimeData
 * @param items
 * @return a pointer on table data
 */
QMimeData* ImportDataTable::mimeData(const QList<QTableWidgetItem*> items) const
{
    QMimeData* mimeData = new QMimeData();

    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    int row = -1;
    QStringList itemStr;

    const QString csvSep = AppSettings::mCSVCellSeparator;

    foreach (QTableWidgetItem* item, items) {
        if (item) {
            if (item->row() != row) {
                if (!itemStr.empty())
                    stream << itemStr.join(csvSep);

                itemStr.clear();
                row = item->row();
                itemStr << QString::number(row);

                //QString pluginName = verticalHeaderItem(row)->text();
                // itemStr << pluginName;
                QString evenName = verticalHeaderItem(row)->text();
                itemStr << evenName;

            }
            QString text = item->text();
            itemStr << text;
        }
    }
    if (!itemStr.empty())
        stream << itemStr.join(csvSep);

    mimeData->setData("application/chronomodel.import.data", encodedData);
    return mimeData;
}

void ImportDataTable::updateTableHeaders()
{
    QList<QTableWidgetItem*> items = selectedItems();
    QString pluginName;
    QString verticalHeader;
    for (int i = 0; i<items.size(); ++i) {
        QString curPluginName = item(items[i]->row(), 0)->text();
        if (pluginName.isEmpty()) {
            pluginName = curPluginName;
            verticalHeader = verticalHeaderItem(items[i]->row())->text();

        } else if (pluginName != curPluginName) {
            pluginName = QString();
            break;
        }
    }

    QStringList headers;
    int numCols = columnCount();

    if (!pluginName.isEmpty() && (verticalHeader!="TITLE")  && (verticalHeader!="STRUCTURE") && (pluginName.toLower()!="bound")) {
        PluginAbstract* plugin = PluginManager::getPluginFromName(pluginName);
        headers << "Method";
        headers << plugin->csvColumns();
        if (plugin->wiggleAllowed()) {
            headers << "Wiggle Type (none | fixed | range | gaussian)";
            headers << "Wiggle value 1 (fixed | Lower date | Average)";
            headers << "Wiggle value 2 (Upper date | Error)";
        }
        
        int chronocurveStartIndex = 14;
        while (headers.size() < numCols) {
            if (headers.size() == chronocurveStartIndex) {
                headers << "Y";
            } else if (headers.size() == (chronocurveStartIndex + 1)) {
                headers << "Error (Y)";
            } else if (headers.size() == (chronocurveStartIndex + 2)) {
                headers << "Inc";
            } else if (headers.size() == (chronocurveStartIndex + 3)) {
                headers << "Dec";
            } else if (headers.size() == (chronocurveStartIndex + 4)) {
                headers << "Error (inc)";
            } else if (headers.size() > chronocurveStartIndex) {
                headers << "Comment";
            } else {
                // Empty values between dates and chronocurve
                headers << "";
            }
        }

    } else if (pluginName.toLower()=="bound") {
        QStringList cols;
        headers << "Bound";
        headers << "Value";
        for (int i = 1; i < numCols; i++)
            cols<<"";
        headers << cols;

    } else if ((verticalHeader!="TITLE")  || (verticalHeader!="STRUCTURE")) {
        QStringList cols;
        cols << "Info";
        for (int i = 1; i < numCols; i++)
            cols<<"";
        headers = cols;

    } else {
        while(headers.size() < numCols)
            headers << "?";
    }
    setHorizontalHeaderLabels(headers);
}
