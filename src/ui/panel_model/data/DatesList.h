#ifndef DatesList_H
#define DatesList_H

#include <QListWidget>
#include <QJsonObject>

class QListWidgetItem;
class Event;
class Date;


class DatesList: public QListWidget
{
    Q_OBJECT
public:
    DatesList(QWidget* parent = 0);
    ~DatesList();
    
    void setEvent(const QJsonObject& event);
    
protected slots:
    void handleItemClicked(QListWidgetItem* item);
    void handleItemDoubleClicked(QListWidgetItem* item);
    
    void forceAtLeastOneSelected();
    
signals:
    void calibRequested(const QJsonObject& date);
    
protected:
    void dropEvent(QDropEvent* e);
    
private:
    QJsonObject mEvent;
    QList<QListWidgetItem*> mSelectedItems;
    bool mUpdatingSelection;
};

#endif
