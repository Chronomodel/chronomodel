/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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
#include "PluginAbstract.h"
#include "QtUtilities.h"
#include "HelpWidget.h"
#include "Button.h"
#include "MainWindow.h"
#include "Project.h"
#include "CurveSettings.h"
#include "AppSettings.h"

#include <QtWidgets>


ImportDataView::ImportDataView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    mBrowseBut = new Button(tr("Load CSV file..."), this);
    mExportBut = new Button(tr("Export all project data as CSV"), this);
    mHelp = new HelpWidget(this);
    mHelp->setLink("https://chronomodel.com/storage/medias/59_manuel_release_2_0_version_1_04_03_2019.pdf#page=31"); //chapter 3.4.2.1 Radiocarbon dating (14C)

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
 * @brief EventsScene::decodeDataDrop insert Event when drop a CSV line from the table importCSV
 * the table may be come from ImportDataView::exportDates()
 *  Since version 3.1.3
 * @param e
 * @example
 * Maximal number of columns used in ChronoModel;
 * // this is a comment :test file for csv with Event's name
Title; Toy
//1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16;17;18;19;20
Structure;Event 1;
// Event name;Dating method;dating name/code;Age;error;calibration curve;Reservoir R;delta R;"wiggle matching
""fixed""
""range""
""gaussian""";wiggle value 1;Wiggle value 2;;;;X_Inc_Depth;Err X- apha95- Err depth;Y_Declinaison;Err Y;Z_Field;Err Z_Err F;
Event name 1;14C;14C_Ly_5212;1370;50;intcal20.14c;0;0;none;;;;;;74;5;50;-10;2;;
Event name 2;14C;14C_Ly_5212;1370;50;intcal20.14c;0;0;gaussian;30;5;;;;;;;;;;
// Event name;methode;dating name/code;Age;error;"reference year
(for measurement)";;;;;;;;;prof;err prof;;;;;
Event name 3;TL/OSL;TL-CLER-202a;987;120;1990;;;;;;;;;220;3;;;;;
Event name 4;TL/OSL;TL-CLER-202b;1170;140;1990;;;;;;;;;;;;;;;
Event name 5;TL/OSL;TL-CLER-203;1280;170;1990;;;;;;;;;;;;;;;
// Event name;methode;dating name/code;measurement type;mean value;Inclination  value corresponding to declination;colonne inutile !;"std error
alpha95";Reference Curve;;;;;;;;;;;;
Event name 6;AM;kiln A;inclination;65;0;0;2,5;FranceInc;;;;;;;;;;;;
Event name 7;AM;kiln A;declination;-20;65;0;2,5;FranceDec;;;;;;;;;;;;
Event name 8;AM;kiln A;intensity;53;0;53;5;FranceInt;;;;;;;;;;;;
// Event name;methode;dating name/code;mean;error;calibration curve;param a;param b;param c;"wiggle matching
""fixed""
""range""
""gaussian""";wiggle value 1;Wiggle value 2;;;;;;;;;
Event name 9;GAUSS;date 1;1000;50;none;;;;;;;;;;;;;;;
Event name 10;GAUSS;date 1;1000;50;none;;;;;;;;;;;;;;;
Event name 11;GAUSS;date 1;1000;50;ReferenceCurveName;;;;;;;;;;;;;;;
Event name 12;GAUSS;date 2;1000;50;equation;0,01;-1;-1000;fixed;20;;;;;;;;;;
Event name 13;GAUSS;date 2;1000;50;equation;0,01;-1;-1000;range;10;15;;;;;;;;;
// Event name;methode;dating name/code;date t1;date t2;;;;;;;;;;;;;;;;
Event name 14;UNIF;date archéo ;300;500;;;;;;;;;;;;;;;;
// Bound
Bound name;Bound;1800;;;;;;;;;;;;0;2;0;0;0;0
 * @return
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
            QStringList pluginNames = PluginManager::getPluginsNames();
            pluginNames.append("bound");

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

                    if (values.size() > 2 && values.at(0) == "") {
                        values[0]="No Name "+ QString::number(noNameCount);
                        ++noNameCount;
                    }
                    if (isComment(values.at(0))) {
                        continue;

                    } else if (values.at(0).contains("title", Qt::CaseInsensitive)) {
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
                QMessageBox message(QMessageBox::Warning, tr("Bad file"), tr("Maybe you need to check the manual to build your CSV file !") + " <a href='https://chronomodel.com/storage/medias/59_manuel_release_2_0_version_1_04_03_2019.pdf#page=29 '>"+ tr("More...")+"</a> ",
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
 *  We can insert the dates later with EventsScene::decodeDataDrop
 */
void ImportDataView::exportDates()
{
    QString currentDir = MainWindow::getInstance()->getCurrentPath();
    QString path = QFileDialog::getSaveFileName(qApp->activeWindow(), tr("Save as CSV"), currentDir, "CSV File (*.csv)");

    if (!path.isEmpty()) {
        QFileInfo info(path);
        mPath = info.absolutePath();
        MainWindow::getInstance()->setCurrentPath(mPath);

        QString sep = AppSettings::mCSVCellSeparator;
        QLocale csvLocal = AppSettings::mCSVDecSeparator == "." ? QLocale::English : QLocale::French;
        csvLocal.setNumberOptions(QLocale::OmitGroupSeparator);

        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);

            Project* project = MainWindow::getInstance()->getProject();
            QJsonArray events = project->mState.value(STATE_EVENTS).toArray();

            stream << "Title" << sep << AppSettings::mLastFile << Qt::endl;
            stream <<"#Event name"<<sep<<"method"<<sep<<"dating name/code"<<sep<<"method param a"<<sep<<"method param b"<<sep<<"method param c"<<sep<<"wiggle type"<<sep<<"wiggle param a"<<sep<<"wiggle param b";
            stream<<sep<<sep<<sep<<sep<<sep<<sep<<"X_Inc_Depth"<<sep<<"Err X- apha95- Err depth"<<sep<<"Y_Declinaison"<<sep<<"Err Y"<<sep<<"Z_Field"<<sep<<"Err Z_Err F" <<Qt::endl;
             bool isCurve = (project->mState.value(STATE_CURVE).toObject().value(STATE_CURVE_PROCESS_TYPE).toInt() != CurveSettings::eProcessTypeNone);

            for (auto&& ev : events) {
                QJsonObject event = ev.toObject();
                QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();

                const int type = event.value(STATE_EVENT_TYPE).toInt();
                const QString eventName = event.value(STATE_NAME).toString();


                if (type == Event::eBound) {
                    QList<QString> dateCsv;
                    dateCsv.append("Bound");
                    dateCsv.append(csvLocal.toString(event.value(STATE_EVENT_KNOWN_FIXED).toDouble()));

                    if (isCurve) {
                        // Curve values start at column 15.
                        // They must be put from column 14 in dateCsv,
                        // because the row is shifted by one column at inserting eventName (see below)
                        const int CurveStartColumn = 13;
                        while (dateCsv.count() < CurveStartColumn) {
                            dateCsv.append("");
                        }
                        dateCsv.append(csvLocal.toString(event.value(STATE_EVENT_X_INC_DEPTH).toDouble()));
                        dateCsv.append(csvLocal.toString(event.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble()));
                        dateCsv.append(csvLocal.toString(event.value(STATE_EVENT_Y_DEC).toDouble()));
                        dateCsv.append(csvLocal.toString(event.value(STATE_EVENT_SY).toDouble()));
                        dateCsv.append(csvLocal.toString(event.value(STATE_EVENT_Z_F).toDouble()));
                        dateCsv.append(csvLocal.toString(event.value(STATE_EVENT_SZ_SF).toDouble()));

                        stream << eventName << sep;
                        stream << dateCsv.join(sep) << Qt::endl;
                    }
                } else {
                   for (auto&& iDate : dates) {
                        QJsonObject date = iDate.toObject();
                        try {
                            Date d (date);
                            if (!d.isNull()) {
                                QStringList dateCsv = d.toCSV(csvLocal);
                                
                                if (isCurve) {
                                    // Curve values start at column 15.
                                    // They must be put from column 14 in dateCsv,
                                    // because the row is shifted by one column at inserting eventName (see below)
                                    const int CurveStartColumn = 13;
                                    while (dateCsv.count() < CurveStartColumn) {
                                        dateCsv.append("");
                                    }
                                    dateCsv.append(csvLocal.toString(event.value(STATE_EVENT_X_INC_DEPTH).toDouble()));
                                    dateCsv.append(csvLocal.toString(event.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble()));
                                    dateCsv.append(csvLocal.toString(event.value(STATE_EVENT_Y_DEC).toDouble()));
                                    dateCsv.append(csvLocal.toString(event.value(STATE_EVENT_SY).toDouble()));
                                    dateCsv.append(csvLocal.toString(event.value(STATE_EVENT_Z_F).toDouble()));
                                    dateCsv.append(csvLocal.toString(event.value(STATE_EVENT_SZ_SF).toDouble()));
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
    for (auto i = rows.size()-1; i >= 0; --i) {
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
    for (auto i = rows.size()-1; i >= 0; --i) {
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
QMimeData* ImportDataTable::mimeData(const QList<QTableWidgetItem *> &items) const
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

    if (!pluginName.isEmpty() && (verticalHeader!="TITLE")  && (verticalHeader!="STRUCTURE") && (!pluginName.contains("bound", Qt::CaseInsensitive))) {
        PluginAbstract* plugin = PluginManager::getPluginFromName(pluginName);
        if (plugin!=nullptr) {
            headers << "Method";
            headers << plugin->csvColumns();
            if (plugin->wiggleAllowed()) {
                headers << "Wiggle Type (none | fixed | range | gaussian)";
                headers << "Wiggle value 1 (fixed | Lower date | Average)";
                headers << "Wiggle value 2 (Upper date | Error)";
            }
        } /*else {
            return;
        }*/
        const int CurveStartIndex = 13; // +1 for index +1 begin = 15
        while (headers.size() < numCols) {
            if (headers.size() == CurveStartIndex) {
                headers << "X_Inc_Depth";
            } else if (headers.size() == (CurveStartIndex + 1)) {
                headers << "Err X- apha95- Err depth";
            } else if (headers.size() == (CurveStartIndex + 2)) {
                headers << "Y_Dec";
            } else if (headers.size() == (CurveStartIndex + 3)) {
                headers << "Err Y";
            } else if (headers.size() == (CurveStartIndex + 4)) {
                headers << "Z_Field";
            } else if (headers.size() == (CurveStartIndex + 5)) {
                headers << "Err Z_Err F";
            } else if (headers.size() > CurveStartIndex) {
                headers << "Comment";
            } else {
                // Empty values between dates and Curve
                headers << "";
            }
        }

    } else if (pluginName.contains("bound", Qt::CaseInsensitive)) {

        headers << "Bound";
        headers << "Value";

        const int CurveStartIndex = 13; // +1 for index +1 begin = 15
        while (headers.size() < numCols) {
            if (headers.size() == CurveStartIndex) {
                headers << "X_Inc_Depth";
            } else if (headers.size() == (CurveStartIndex + 1)) {
                headers << "Err X- apha95- Err depth";
            } else if (headers.size() == (CurveStartIndex + 2)) {
                headers << "Y_Dec";
            } else if (headers.size() == (CurveStartIndex + 3)) {
                headers << "Err Y";
            } else if (headers.size() == (CurveStartIndex + 4)) {
                headers << "Z_Field";
            } else if (headers.size() == (CurveStartIndex + 5)) {
                headers << "Err Z_Err F";
            } else if (headers.size() > CurveStartIndex) {
                headers << "Comment";
            } else {
                // Empty values between dates and Curve
                headers << "";
            }
        }

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
