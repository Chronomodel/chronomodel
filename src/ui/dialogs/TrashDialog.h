#ifndef TrashDialog_H
#define TrashDialog_H

#include <QDialog>
#include "Date.h"

class QListWidget;
class Button;


class TrashDialog: public QDialog
{
    Q_OBJECT
public:
    enum Type{
        eDate = 0,
        eEvent = 1
    };
    
    TrashDialog(Type type, QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
    ~TrashDialog();
    
    QList<int> getSelectedIndexes();
    
private slots:
    void updateFromSelection();
    void deleteItems(bool checked);
    
public:
    QListWidget* mList;
    Type mType;
    
    Button* mDeleteBut;
    Button* mOkBut;
    Button* mCancelBut;
};

#endif
