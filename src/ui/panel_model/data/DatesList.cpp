#include "DatesList.h"
#include "Phase.h"
#include "Date.h"
#include "Event.h"
#include "MainWindow.h"
#include "Project.h"
#include "../PluginAbstract.h"
#include "DatesListItemDelegate.h"
#include "ModelUtilities.h"
#include "Button.h"
#include <QtWidgets>


DatesList::DatesList(QWidget* parent):QListWidget(parent)
{
    setDragDropMode(QAbstractItemView::InternalMove);
    //setDefaultDropAction(Qt::MoveAction);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSortingEnabled(false);
    
    
    DatesListItemDelegate* delegate = new DatesListItemDelegate();
    setItemDelegate(delegate);
    
    // ----------
    
    connect(this, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(handleItemClicked(QListWidgetItem*)));
    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(handleItemDoubleClicked(QListWidgetItem*)));
}

DatesList::~DatesList()
{
    
}

void DatesList::setEvent(const QJsonObject& event)
{
    mEvent = event;
    
    clear();
    
    if(!mEvent.isEmpty())
    {
        QJsonArray dates = mEvent[STATE_EVENT_DATES].toArray();
        for(int i=0; i<dates.size(); ++i)
        {
            QJsonObject date = dates[i].toObject();
            
            try{
                Date d = Date::fromJson(date);
                if(!d.isNull())
                {
                    QListWidgetItem* item = new QListWidgetItem();
                    item->setFlags(Qt::ItemIsSelectable
                                   | Qt::ItemIsDragEnabled
                                   | Qt::ItemIsUserCheckable
                                   | Qt::ItemIsEnabled
                                   | Qt::ItemNeverHasChildren);
                    
                    item->setText(d.mName);
                    item->setData(0x0101, d.mName);
                    item->setData(0x0102, d.mPlugin->getId());
                    item->setData(0x0103, d.getDesc());
                    item->setData(0x0104, d.mId);
                    item->setData(0x0105, ModelUtilities::getDeltaText(d));
                    item->setData(0x0106, ModelUtilities::getDataMethodText(d.mMethod));
                    item->setData(0x0107, d.mIsValid);
                    item->setData(0x0108, date[STATE_DATE_SUB_DATES].toArray().size() > 0);
                    
                    addItem(item);
                }
            }
            catch(QString error){
                QMessageBox message(QMessageBox::Critical,
                                    qApp->applicationName() + " " + qApp->applicationVersion(),
                                    tr("Error : ") + error,
                                    QMessageBox::Ok,
                                    qApp->activeWindow(),
                                    Qt::Sheet);
                message.exec();
            }
        }
    }
}

void DatesList::handleItemClicked(QListWidgetItem* item)
{
    /*QFont font;
    font.setPointSizeF(pointSize(11));
    QFontMetrics metrics(font);
    int mm = 2;
    int mh = metrics.height();
    int butW = 60;
    int butH = 20;
    int iconW = 30;
    int rowH = 4*mh + 6*mm + butH;
    
    QPoint pos = mapFromGlobal(QCursor::pos());
    int scrollOffset = verticalScrollBar()->value() * rowH;
    pos.setY(pos.y() + scrollOffset);
    
    int index = row(item);
    int yOffset = index * rowH;
    
    QRect updateRect(iconW, yOffset + rowH - mm - butH, butW, butH);
    QRect calibRect(iconW + butW + 2*mm, yOffset + rowH - mm - butH, butW, butH);
    
    //qDebug() << "----";
    //qDebug() << "index : " << index << ", y dans [" << (index * rowH) << ", " << ((index+1) * rowH) << "]";
    //qDebug() << "scroll offset : " << scrollOffset;
    //qDebug() << "position in scrolled widget : " << pos;
    
    if(updateRect.contains(pos))
    {
        //qDebug() << "update clicked for item at : " << index;
        MainWindow::getInstance()->getProject()->updateDate(mEvent[STATE_ID].toInt(), index);
    }
    else if(calibRect.contains(pos))
    {
        //qDebug() << "calib clicked for item at : " << index;
        QJsonArray dates = mEvent[STATE_EVENT_DATES].toArray();
        if(index < dates.size())
        {
            QJsonObject date = dates[index].toObject();
            emit calibRequested(date);
        }
    }*/
    
    
    int index = row(item);
    QJsonArray dates = mEvent[STATE_EVENT_DATES].toArray();
    if(index < dates.size())
    {
        QJsonObject date = dates[index].toObject();
        emit calibRequested(date);
    }
}

void DatesList::handleItemDoubleClicked(QListWidgetItem* item)
{
    //Q_UNUSED(item);
    if(!mEvent.isEmpty())
    {
        MainWindow::getInstance()->getProject()->updateDate(mEvent[STATE_ID].toInt(), row(item));
    }
}

void DatesList::dropEvent(QDropEvent* e)
{
    QListWidget::dropEvent(e);
    
    QList<int> ids;
    for(int i=0; i<count(); ++i)
    {
        QListWidgetItem* it = item(i);
        int id = it->data(0x0104).toInt();
        ids << id;
    }
    QJsonObject event = mEvent;
    QJsonArray dates = event[STATE_EVENT_DATES].toArray();
    QJsonArray datesOrdered;
    for(int i=0; i<ids.size(); ++i)
    {
        int id = ids[i];
        for(int j=0; j<dates.size(); ++j)
        {
            QJsonObject date = dates[j].toObject();
            int dateId = date[STATE_ID].toInt();
            if(dateId == id)
            {
                datesOrdered.append(date);
                break;
            }
        }
    }
    event[STATE_EVENT_DATES] = datesOrdered;
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Dates order changed"));
}



