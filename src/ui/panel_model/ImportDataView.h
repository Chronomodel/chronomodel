#ifndef ImportDataView_H
#define ImportDataView_H

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
    ImportDataTable(ImportDataView* importView, QWidget* parent = 0);
    virtual ~ImportDataTable();

private slots:
    void updateTableHeaders();
    
protected:
    QMimeData* mimeData(const QList<QTableWidgetItem*> items) const;
    
private:
    ImportDataView* mImportView;
};



class ImportDataView: public QWidget
{
    Q_OBJECT
public:
    ImportDataView(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~ImportDataView();
    
public slots:
    void removeCsvRows(QList<int> rows);
    
private slots:
    void browse();
    void exportDates();
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    
private:
    Button* mBrowseBut;
    Button* mExportBut;
    ImportDataTable* mTable;
    HelpWidget* mHelp;
    
public:
    QString mPath;
};

#endif
