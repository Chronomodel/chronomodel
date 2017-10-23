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
    setWindowTitle(tr("Restore From Trash"));
    
    // -----------
    
    mList = new QListWidget(this);
    mList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mList->setMinimumHeight(400);
    
    QItemDelegate* delegate = nullptr;
    if (mType == eDate)
        delegate = new DatesListItemDelegate();
    else if (mType == eEvent)
        delegate = new EventsListItemDelegate();
    if (delegate)
        mList->setItemDelegate(delegate);
    
    connect(mList, &QListWidget::itemSelectionChanged, this, &TrashDialog::updateFromSelection);
    
    // ----------
    
    mDeleteBut = new Button(tr("Delete"), this);
    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    
    connect(mDeleteBut, &Button::clicked, this, &TrashDialog::deleteItems);
    connect(mOkBut, &Button::clicked, this, &TrashDialog::accept);
    connect(mCancelBut, &Button::clicked, this, &TrashDialog::reject);
    
    QHBoxLayout* butLayout = new QHBoxLayout();
    butLayout->setContentsMargins(0, 0, 0, 0);
    butLayout->setSpacing(25);
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
        const QJsonObject state = project->state();
        const ProjectSettings settings = ProjectSettings::fromJson(state[STATE_SETTINGS].toObject());

        QJsonArray dates = state[STATE_DATES_TRASH].toArray();
        
        for (int i=0; i<dates.size(); ++i) {
            try {
                QJsonObject date = dates[i].toObject();

//                // Validate the date before adding it to the correct event and pushing the state
//                QJsonObject settingsJson = stateNext[STATE_SETTINGS].toObject();
//                ProjectSettings settings = ProjectSettings::fromJson(settingsJson);
                PluginAbstract* plugin = PluginManager::getPluginFromId(date[STATE_DATE_PLUGIN_ID].toString());
                bool valid = plugin->isDateValid(date[STATE_DATE_DATA].toObject(), settings);
                date[STATE_DATE_VALID] = valid;
                Date d;
                d.fromJson(date);

                if (!d.isNull()) {
                    QListWidgetItem* item = new QListWidgetItem(d.mName);

                    item->setText(d.mName);
                    item->setData(0x0101, d.mName);
                    item->setData(0x0102, d.mPlugin->getId());
                    item->setData(0x0103, d.getDesc());
                    item->setData(0x0104, d.mId);
                    item->setData(0x0105, ModelUtilities::getDeltaText(d));
                    item->setData(0x0106, ModelUtilities::getDataMethodText(d.mMethod));
                    item->setData(0x0107, d.mIsValid);
                    item->setData(0x0108, date.value(STATE_DATE_SUB_DATES).toArray().size() > 0);

                    mList->addItem(item);
                }
            }
            catch(QString error){
                QMessageBox message(QMessageBox::Warning,
                                    qApp->applicationName() + " " + qApp->applicationVersion(),
                                    tr("Warning : ") + error,
                                    QMessageBox::Ok,
                                    qApp->activeWindow());
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
    for (auto &&item : items)
        result.push_back(mList->row(item));
    return result;
}

void TrashDialog::updateFromSelection()
{
    QList<QListWidgetItem*> items = mList->selectedItems();
    mDeleteBut->setEnabled(items.count() > 0);
}

void TrashDialog::deleteItems(bool checked)
{
    (void) checked;
    Project* project = MainWindow::getInstance()->getProject();
    QList<QListWidgetItem*> items = mList->selectedItems();
    QList<int> ids;
    
    if (mType == eEvent) {
        for (auto &&item : items)
            ids.append(item->data(0x0107).toInt());

        project->deleteSelectedTrashedEvents(ids);
    } else if (mType == eDate) {
        for (auto &&item : items)
            ids.append(item->data(0x0106).toInt());

        project->deleteSelectedTrashedDates(ids);
    }
    
    /* Delete items now!
    *  An event has been sent to the app to destroy these items, but our dialog cannot listen to the notification that will be sent after (at least not for now...)
    *  Deleting items now is thus a bit anticipated but works well!
    */
    for (auto &&item : items)
        mList->takeItem(mList->row(item));
    mList->update();
}


