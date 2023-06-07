/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2020

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
    connect(this, &DatesList::itemSelectionChanged, this, &DatesList::handleItemIsChanged);
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
        for (int i = 0; i<dates.size(); ++i) {
            QJsonObject date = dates[i].toObject();

            try {
                Date d (date);
                
                if (!d.isNull()) {
                    QListWidgetItem* item = new QListWidgetItem();
                    item->setFont(font());
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
                    item->setData(0x0105, d.getWiggleDesc()); //ModelUtilities::getDeltaText(d));
                    item->setData(0x0106, MHVariable::getSamplerProposalText(d.mTi.mSamplerProposal));
                    item->setData(0x0107, d.mIsValid);
                    item->setData(0x0108, date.value(STATE_DATE_SUB_DATES).toArray().size() > 0);
                    item->setData(0x0109, d.mOrigin);
                    item->setData(0x0110, d.mUUID);
                    //item->setData(0x0111, d.getWiggleDesc());
                    
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
        emit indexChange(index);
        emit calibRequested(date);
    }
}

void DatesList::handleItemIsChanged()
{
    QJsonArray dates = mEvent[STATE_EVENT_DATES].toArray();
    for (int i = 0; i <dates.size(); ++i ) {
        if (item(i)->isSelected() ) {

            QJsonObject date = dates[i].toObject();
            emit indexChange(i);
            emit calibRequested(date);
            break;
        }
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
    for (int i = 0; i<count(); ++i) {
        QListWidgetItem* it = item(i);
        int id = it->data(0x0104).toInt();
        ids << id;
    }
    
    QJsonObject event = mEvent;
    QJsonArray dates = event[STATE_EVENT_DATES].toArray();
    QJsonArray datesOrdered;
    for (int i = 0; i<ids.size(); ++i) {
        const int id = ids[i];

        for (int j = 0; j<dates.size(); ++j){
            QJsonObject date = dates[j].toObject();
            int dateId = date[STATE_ID].toInt();
            if (dateId == id) {
                datesOrdered.append(date);
                break;
            }
        }
    }
    event[STATE_EVENT_DATES] = datesOrdered;
    MainWindow::getInstance()->updateEvent(event, tr("Dates order changed"));
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
        for (auto&& it : mSelectedItems) {
            it->setSelected(true);

        }
        mUpdatingSelection = false;

    }

}
