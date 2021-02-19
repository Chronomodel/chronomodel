/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

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
    connect(mOkBut,static_cast<void (Button::*)(bool)> (&Button::clicked), this, &TrashDialog::accept);
    connect(mCancelBut, static_cast<void (Button::*)(bool)> (&Button::clicked), this, &TrashDialog::reject);

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

        for (int i(0); i<dates.size(); ++i) {
            try {
                QJsonObject date = dates[i].toObject();

                // Validate the date before adding it to the correct event and pushing the state
                PluginAbstract* plugin = PluginManager::getPluginFromId(date[STATE_DATE_PLUGIN_ID].toString());
                bool valid = plugin->isDateValid(date[STATE_DATE_DATA].toObject(), settings);
                date[STATE_DATE_VALID] = valid;
                Date d(date);

                if (!d.isNull()) {
                    QListWidgetItem* item = new QListWidgetItem(d.mName);

                    item->setText(d.mName);
                    item->setData(0x0101, d.mName);
                    item->setData(0x0102, d.mPlugin->getId());
                    item->setData(0x0103, d.getDesc());
                    item->setData(0x0104, d.mId);
                    item->setData(0x0105, d.getWiggleDesc());//ModelUtilities::getDeltaText(d));
                    item->setData(0x0106, ModelUtilities::getDataMethodText(d.mMethod));
                    item->setData(0x0107, d.mIsValid);
                    item->setData(0x0108, date.value(STATE_DATE_SUB_DATES).toArray().size() > 0);
                    item->setData(0x0109, d.mOrigin);
                    item->setData(0x0110, d.mUUID);

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
            if ((Event::Type) event.value(STATE_EVENT_TYPE).toInt() == Event::Type::eDefault) {
                item->setData(0x0103, event[STATE_EVENT_DATES].toArray().size());
            } else {
                item->setData(0x0103, -1);
            }

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
