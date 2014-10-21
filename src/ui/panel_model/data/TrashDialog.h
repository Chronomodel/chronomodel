#ifndef TrashDialog_H
#define TrashDialog_H

#include <QDialog>
#include "Date.h"

class QListWidget;


class TrashDialog: public QDialog
{
    Q_OBJECT
public:
    enum Type{
        eDate = 0,
        eEvent = 1
    };
    
    TrashDialog(Type type, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~TrashDialog();
    
    QList<int> getSelectedIndexes();
    
public:
    QListWidget* mList;
    Type mType;
};

#endif
