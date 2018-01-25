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


DatesList::DatesList(QWidget* parent):QListWidget(parent),
mUpdatingSelection(false)
{
    setDragDropMode(QAbstractItemView::InternalMove);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSortingEnabled(false);
    
    DatesListItemDelegate* delegate = new DatesListItemDelegate(this);
    setItemDelegate(delegate);
    
    connect(this, &DatesList::itemClicked, this, &DatesList::handleItemClicked);
    connect(this, &DatesList::itemDoubleClicked, this, &DatesList::handleItemDoubleClicked);
    connect(this, &DatesList::itemSelectionChanged, this, &DatesList::forceAtLeastOneSelected);
}

DatesList::~DatesList()
{
    
}

void DatesList::setEvent(const QJsonObject& event)
{
    mEvent = event;
    
    clear();
    
    if (!mEvent.isEmpty()) {
        QJsonArray dates = mEvent[STATE_EVENT_DATES].toArray();
        for (int i(0); i<dates.size(); ++i) {
            QJsonObject date = dates[i].toObject();
            
            try {
                Date d;
                d.fromJson(date);
                if (!d.isNull()) {
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
                    item->setData(0x0108, date.value(STATE_DATE_SUB_DATES).toArray().size() > 0);
                    
                    addItem(item);
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
        if (dates.size() > 0) {
            // Select first date by default :
            int idx = this->currentRow();
            if (idx>=0) {
                setCurrentRow(idx);
                // Prepare calib window :
                handleItemClicked(item(idx));
            }
        }
    }
}

void DatesList::handleItemClicked(QListWidgetItem* item)
{
    int index = row(item);
    QJsonArray dates = mEvent[STATE_EVENT_DATES].toArray();
    if (index < dates.size()) {
        QJsonObject date = dates[index].toObject();
        emit calibRequested(date);
    }
}

void DatesList::handleItemDoubleClicked(QListWidgetItem* item)
{
    if (!mEvent.isEmpty() && item)
        MainWindow::getInstance()->getProject()->updateDate(mEvent[STATE_ID].toInt(), row(item));

}

void DatesList::dropEvent(QDropEvent* e)
{
    QListWidget::dropEvent(e);
    
    QList<int> ids;
    for (int i=0; i<count(); ++i) {
        QListWidgetItem* it = item(i);
        int id = it->data(0x0104).toInt();
        ids << id;
    }
    QJsonObject event = mEvent;
    QJsonArray dates = event[STATE_EVENT_DATES].toArray();
    QJsonArray datesOrdered;
    for (int i=0; i<ids.size(); ++i) {
        int id = ids[i];
        for (int j=0; j<dates.size(); ++j){
            QJsonObject date = dates[j].toObject();
            int dateId = date[STATE_ID].toInt();
            if (dateId == id) {
                datesOrdered.append(date);
                break;
            }
        }
    }
    event[STATE_EVENT_DATES] = datesOrdered;
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Dates order changed"));
}

void DatesList::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Return)
        handleItemDoubleClicked(currentItem());

    else
        QListWidget::keyPressEvent(e);

}

void DatesList::forceAtLeastOneSelected()
{
    if (selectedItems().size() > 0) {
        if (!mUpdatingSelection)
            mSelectedItems = selectedItems();

    } else {
        mUpdatingSelection = true;
        for (auto && it:mSelectedItems)
            it->setSelected(true);

        mUpdatingSelection = false;
    }
}
