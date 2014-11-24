#include "TrashDialog.h"
#include "DatesListItemDelegate.h"
#include "EventsListItemDelegate.h"
#include "../PluginAbstract.h"
#include "../PluginFormAbstract.h"
#include "MainWindow.h"
#include "Project.h"
#include <QtWidgets>


TrashDialog::TrashDialog(Type type, QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
mType(type)
{
    setWindowTitle(tr("Restore from trash"));
    
    // -----------
    
    mList = new QListWidget();
    mList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mList->setMinimumHeight(400);
    
    
    QItemDelegate* delegate = 0;
    if(mType == eDate)
        delegate = new DatesListItemDelegate();
    else if(mType == eEvent)
        delegate = new EventsListItemDelegate();
    if(delegate)
        mList->setItemDelegate(delegate);
    
    // ----------
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox();
    buttonBox->addButton(tr("OK"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
    
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    
    // ----------
    
    QFont font;
    font.setWeight(QFont::Bold);

    QLabel* titleLab = new QLabel(tr("Select the item to be restored") + " :");
    titleLab->setFont(font);
    titleLab->setAlignment(Qt::AlignCenter);
    
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    
    QLabel* intro = new QLabel(tr("<p>TODO...</p>"));
    intro->setTextFormat(Qt::RichText);
    intro->setWordWrap(true);
    
    // ----------
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(titleLab);
    //layout->addWidget(intro);
    //layout->addWidget(separator);
    layout->addWidget(mList);
    layout->addWidget(buttonBox);
    setLayout(layout);
    
    // ----------
    
    Project* project = MainWindow::getInstance()->getProject();
    
    if(mType == eDate)
    {
        QJsonObject state = project->state();
        QJsonArray dates = state[STATE_DATES_TRASH].toArray();
        
        for(int i=0; i<dates.size(); ++i)
        {
            QJsonObject date = dates[i].toObject();
            Date d = Date::fromJson(date);
            
            QListWidgetItem* item = new QListWidgetItem(d.mName);
            item->setData(0x0101, d.mName);
            item->setData(0x0102, d.mPlugin->getId());
            item->setData(0x0103, d.getDesc());
            item->setData(0x0105, d.mDeltaFixed);
            mList->addItem(item);
        }
    }
    else if(mType == eEvent)
    {
        QJsonObject state = project->state();
        QJsonArray events = state[STATE_EVENTS_TRASH].toArray();
        
        for(int i=0; i<events.size(); ++i)
        {
            QJsonObject event = events[i].toObject();
            QListWidgetItem* item = new QListWidgetItem(event[STATE_NAME].toString());
            item->setData(0x0101, event[STATE_NAME].toString());
            item->setData(0x0103, event[STATE_EVENT_DATES].toArray().size());
            item->setData(0x0104, event[STATE_COLOR_RED].toInt());
            item->setData(0x0105, event[STATE_COLOR_GREEN].toInt());
            item->setData(0x0106, event[STATE_COLOR_BLUE].toInt());
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
    for(int i=0; i<items.size(); ++i)
        result.push_back(mList->row(items[i]));
    return result;
}

