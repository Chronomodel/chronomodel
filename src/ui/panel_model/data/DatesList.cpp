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
                
                addItem(item);
                
                /*Button* updateBut = new Button(tr("Update"));
                Button* calibBut = new Button(tr("Calibrate"));
                
                updateBut->setMaximumSize(70, 25);
                updateBut->setGeometry(100, 10, 70, 25);
                
                setItemWidget(item, updateBut);
                //setItemWidget(item, calibBut);*/
            }
        }
    }
}

void DatesList::handleItemClicked(QListWidgetItem* item)
{
    QFont font;
    font.setPointSizeF(pointSize(11));
    QFontMetrics metrics(font);
    int mm = 2;
    int mh = metrics.height();
    int butW = 60;
    int butH = 20;
    int iconW = 30;
    int rowH = 4*mh + 6*mm + butH;
    
    QPoint pos = mapFromGlobal(QCursor::pos());
    int index = row(item);
    int yOffset = index * rowH;
    
    //pos.setY(pos.y() - yOffset);
    //qDebug() << yOffset << ", " << pos;
    
    QRect updateRect(iconW, yOffset + rowH - mm - butH, butW, butH);
    QRect calibRect(iconW + butW + 2*mm, yOffset + rowH - mm - butH, butW, butH);
    
    /*qDebug() << "----";
    qDebug() << "index : " << index;
    qDebug() << "rowH : " << rowH;
    qDebug() << "yOffset : " << yOffset;
    qDebug() << "pos : " << pos;
    qDebug() << "updateRect : " << updateRect;*/
    
    if(updateRect.contains(pos))
    {
        MainWindow::getInstance()->getProject()->updateDate(mEvent[STATE_ID].toInt(), index);
    }
    else if(calibRect.contains(pos))
    {
        QJsonArray dates = mEvent[STATE_EVENT_DATES].toArray();
        if(index < dates.size())
        {
            QJsonObject date = dates[index].toObject();
            emit calibRequested(date);
        }
    }
    
    QJsonArray dates = mEvent[STATE_EVENT_DATES].toArray();
    if(index < dates.size())
    {
        QJsonObject date = dates[index].toObject();
        emit MainWindow::getInstance()->getProject()->currentDateChanged(date);
    }
}

void DatesList::handleItemDoubleClicked(QListWidgetItem* item)
{
    Q_UNUSED(item);
    /*if(!mEvent.isEmpty())
    {
        MainWindow::getInstance()->getProject()->updateDate(mEvent[STATE_ID].toInt(), row(item));
    }*/
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



