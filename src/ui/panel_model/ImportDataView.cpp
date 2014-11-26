#include "ImportDataView.h"
#include "PluginManager.h"
#include "../PluginAbstract.h"
#include "QtUtilities.h"
#include "HelpWidget.h"
#include "Button.h"
#include "Label.h"
#include "MainWindow.h"
#include <QtWidgets>


ImportDataView::ImportDataView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    mBrowseBut = new Button(tr("Load CSV file..."), this);
    mHelp = new HelpWidget(this);
    
    mHelp->setText(tr("Your CSV file must contain 1 data per row. Each row must start with the datation method to use. Allowed datation methods are : 14C, AM, Gauss, Typo, TL/OSL.\nComments are allowed in your CSV. They must start with  # or // and can be placed at the end of a data row. When placed at the begining of a row, the whole row is ignored."));
    
    mTable = new ImportDataTable(this, this);
    mTable->setAlternatingRowColors(true);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTable->setDragEnabled(true);
    mTable->setDragDropMode(QAbstractItemView::DragOnly);
    
    connect(mBrowseBut, SIGNAL(clicked()), this, SLOT(browse()));
}

ImportDataView::~ImportDataView()
{
    
}

void ImportDataView::browse()
{
    QString currentDir = MainWindow::getInstance()->getCurrentPath();
    QString path = QFileDialog::getOpenFileName(qApp->activeWindow(), tr("Open CSV File"), currentDir, tr("CSV File (*.csv)"));
    
    if(!path.isEmpty())
    {
        QFileInfo info(path);
        mPath = info.absolutePath();
        MainWindow::getInstance()->setCurrentPath(mPath);
        
        while(mTable->rowCount() > 0)
            mTable->removeRow(0);
        
        QFile file(path);
        if(file.open(QIODevice::ReadOnly))
        {
            QTextStream stream(&file);
            // TODO
            QTextCodec* codec = stream.codec();
            QList<QStringList> data;
            
            int rows = 0;
            int cols = 0;
            
            QStringList headers;
            QStringList pluginNames = PluginManager::getPluginsNames();
            
            // Read every lines of the file
            while(!stream.atEnd())
            {
                QString line = stream.readLine();
                QStringList values = line.split(",");
                if(values.size() > 0)
                {
                    if(isComment(values[0]))
                    {
                        // Comment line found : do not display it
                    }
                    else if(values.size() > 0)
                    {
                        // Display the line only if we have a plugin to import it !
                        QString pluginName = values[0].simplified().toUpper();
                        if(pluginNames.contains(pluginName, Qt::CaseInsensitive))
                        {
                            headers << pluginName;
                            data << values;
                            
                            // Adapt max columns count if necessary
                            cols = (values.size() > cols) ? values.size() : cols;
                            ++rows;
                        }
                    }
                }
            }
            file.close();
            
            mTable->setRowCount(rows);
            mTable->setColumnCount(cols);
            
            for(int i=0; i<data.size(); ++i)
            {
                QStringList d = data[i];
                qDebug() << d;
                for(int j=0; j<d.size(); ++j)
                {
                    // Skip the first column containing the plugin name (already used in the table line header)
                    if(j != 0)
                    {
                        QTableWidgetItem* item = new QTableWidgetItem(d[j].simplified());
                        mTable->setItem(i, j-1, item);
                    }
                }
            }
            // Each line has a header containing the plugin name to use
            mTable->setVerticalHeaderLabels(headers);
            mTable->setCurrentCell(0, 0);
        }
    }
}

void ImportDataView::removeCsvRows(QList<int> rows)
{
    qDebug() << rows;
    sortIntList(rows);
    qDebug() << rows;
    for(int i=rows.size()-1; i>=0; --i)
    {
        qDebug() << "Removing row : " << rows[i];
        mTable->removeRow(rows[i]);
    }
}

void ImportDataView::setHelp(const QString& help)
{
    //mHelp->setText(help);
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
    
    int m = 5;
    int labW = 100;
    int butH = 25;
    int helpH = mHelp->heightForWidth(width() - 2*m);
    
    mBrowseBut->setGeometry(m, m, width() - 2*m, butH);
    mTable->setGeometry(0, 2*m + butH, width(), height() - 4*m - butH - helpH);
    mHelp->setGeometry(m, height() - helpH - m, width() - 2*m, helpH);
}

// ------------------------------------------------------------------------------------

#pragma mark Table

ImportDataTable::ImportDataTable(ImportDataView* importView, QWidget* parent):QTableWidget(parent),
mImportView(importView)
{
    connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(updateTableHeaders()));
}

ImportDataTable::~ImportDataTable()
{

}

QMimeData* ImportDataTable::mimeData(const QList<QTableWidgetItem*> items) const
{
    QMimeData* mimeData = new QMimeData();
    
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    
    int row = -1;
    QStringList itemStr;
    
    foreach(QTableWidgetItem* item, items)
    {
        if(item)
        {
            if(item->row() != row)
            {
                if(!itemStr.empty())
                    stream << itemStr.join(";");
                
                itemStr.clear();
                row = item->row();
                itemStr << QString::number(row);
                
                QString pluginName = verticalHeaderItem(row)->text();
                itemStr << pluginName;
            }
            QString text = item->text();
            itemStr << text;
        }
    }
    if(!itemStr.empty())
        stream << itemStr.join(";");
    
    mimeData->setData("application/chronomodel.import.data", encodedData);
    return mimeData;
}

void ImportDataTable::updateTableHeaders()
{
    QList<QTableWidgetItem*> items = selectedItems();
    QString pluginName;
    for(int i=0; i<items.size(); ++i)
    {
        QString curPluginName = verticalHeaderItem(items[i]->row())->text();
        if(pluginName.isEmpty())
            pluginName = curPluginName;
        else if(pluginName != curPluginName)
        {
            pluginName = QString();
            break;
        }
    }
    
    QStringList headers;
    int numCols = columnCount();
    
    if(!pluginName.isEmpty())
    {
        qDebug() << pluginName;
        PluginAbstract* plugin = PluginManager::getPluginFromName(pluginName);
        mImportView->setHelp(plugin->csvHelp());
        
        headers = plugin->csvColumns();
        if(plugin->wiggleAllowed())
        {
            headers << "Wiggle Type (fixed | range | gaussian)";
            headers << "Wiggle value 1 (fixed | Lower date | Average)";
            headers << "Wiggle value 2 (Upper date | Error)";
        }
        while(headers.size() < numCols)
            headers << "comment";
    }
    else
    {
        while(headers.size() < numCols)
            headers << "?";
    }
    setHorizontalHeaderLabels(headers);
}
