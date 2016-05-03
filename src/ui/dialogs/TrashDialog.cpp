#include "TrashDialog.h"
#include "DatesListItemDelegate.h"
#include "EventsListItemDelegate.h"
#include "../PluginAbstract.h"
#include "../PluginFormAbstract.h"
#include "MainWindow.h"
#include "Button.h"
#include "Project.h"
#include <QtWidgets>


TrashDialog::TrashDialog(Type type, QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
mType(type)
{
    setWindowTitle(tr("Restore from trash"));
    
    // -----------
    
    mList = new QListWidget(this);
    mList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mList->setMinimumHeight(400);
    
    QItemDelegate* delegate = 0;
    if (mType == eDate)
        delegate = new DatesListItemDelegate();
    else if (mType == eEvent)
        delegate = new EventsListItemDelegate();
    if (delegate)
        mList->setItemDelegate(delegate);
    
    connect(mList, SIGNAL(itemSelectionChanged()), this, SLOT(updateFromSelection()));
    
    // ----------
    
    mDeleteBut = new Button(tr("Delete"), this);
    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    
    connect(mDeleteBut, SIGNAL(clicked()), this, SLOT(deleteItems()));
    connect(mOkBut, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mCancelBut, SIGNAL(clicked()), this, SLOT(reject()));
    
    QHBoxLayout* butLayout = new QHBoxLayout();
    butLayout->setContentsMargins(0, 0, 0, 0);
    butLayout->setSpacing(5);
    //butLayout->addStretch();
    butLayout->addWidget(mDeleteBut);
    butLayout->addWidget(mOkBut);
    butLayout->addWidget(mCancelBut);
    
    // ----------
    
    QFont font;
    font.setWeight(QFont::Bold);

    QLabel* titleLab = new QLabel(tr("Select the item to be restored") + " :");
    titleLab->setFont(font);
    titleLab->setAlignment(Qt::AlignCenter);
    
    // ----------
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(titleLab);
    layout->addWidget(mList);
    layout->addLayout(butLayout);
    setLayout(layout);
    
    // ----------
    
    Project* project = MainWindow::getInstance()->getProject();
    
    if (mType == eDate) {
        QJsonObject state = project->state();
        QJsonArray dates = state[STATE_DATES_TRASH].toArray();
        
        for (int i=0; i<dates.size(); ++i) {
            try {
                QJsonObject date = dates[i].toObject();
                Date d = Date::fromJson(date);
                if (!d.isNull()) {
                    QListWidgetItem* item = new QListWidgetItem(d.mName);
                    item->setData(0x0101, d.mName);
                    item->setData(0x0102, d.mPlugin->getId());
                    item->setData(0x0103, d.getDesc());
                    item->setData(0x0105, d.mDeltaFixed);
                    item->setData(0x0106, d.mId);
                    mList->addItem(item);
                }
            }
            catch(QString error){
                QMessageBox message(QMessageBox::Warning,
                                    qApp->applicationName() + " " + qApp->applicationVersion(),
                                    tr("Warning : ") + error,
                                    QMessageBox::Ok,
                                    qApp->activeWindow(),
                                    Qt::Sheet);
                message.exec();
            }
        }
    } else if (mType == eEvent) {
        QJsonObject state = project->state();
        QJsonArray events = state[STATE_EVENTS_TRASH].toArray();
        
        for (int i=0; i<events.size(); ++i) {
            QJsonObject event = events[i].toObject();
            QListWidgetItem* item = new QListWidgetItem(event[STATE_NAME].toString());
            item->setData(0x0101, event[STATE_NAME].toString());
            item->setData(0x0103, event[STATE_EVENT_DATES].toArray().size());
            item->setData(0x0104, event[STATE_COLOR_RED].toInt());
            item->setData(0x0105, event[STATE_COLOR_GREEN].toInt());
            item->setData(0x0106, event[STATE_COLOR_BLUE].toInt());
            item->setData(0x0107, event[STATE_ID].toInt());
            mList->addItem(item);
        }
    }
}

TrashDialog::~TrashDialog()
{
    
}

QList<int> TrashDialog::getSelectedIndexes()
{
    QList<QListWidgetItem*> items = mList->selectedItems();
    QList<int> result;
    for (int i=0; i<items.size(); ++i)
        result.push_back(mList->row(items[i]));
    return result;
}

void TrashDialog::updateFromSelection()
{
    QList<QListWidgetItem*> items = mList->selectedItems();
    mDeleteBut->setEnabled(items.count() > 0);
}

void TrashDialog::deleteItems()
{
    Project* project = MainWindow::getInstance()->getProject();
    QList<QListWidgetItem*> items = mList->selectedItems();
    QList<int> ids;
    
    if (mType == eEvent) {
        for (int i=0; i<items.size(); ++i)
            ids.append(items[i]->data(0x0107).toInt());
        project->deleteSelectedTrashedEvents(ids);
    } else if (mType == eDate) {
        for (int i=0; i<items.size(); ++i)
            ids.append(items[i]->data(0x0106).toInt());
        project->deleteSelectedTrashedDates(ids);
    }
    
    // Delete items now!
    // An event has been sent to the app to destroy these items, but our dialog cannot listen to the notification that will be sent after (at least not for now...)
    // Deleting items now is thus a bit anticipated but works well!
    for (int i=0; i<items.size(); ++i)
        mList->takeItem(mList->row(items[i]));
    mList->update();
}


