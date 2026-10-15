/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#include "DatesList.h"
#include "Date.h"
#include "Event.h"
#include "MainWindow.h"
#include "Project.h"
#include "PluginAbstract.h"
#include "DatesListItemDelegate.h"

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
    //connect(this, &DatesList::itemSelectionChanged, this, &DatesList::handleItemIsChanged);
}

DatesList::~DatesList()
{

}

void DatesList::setEvent(const QJsonObject event)
{
    mEvent = event;

    clear();

    if (!mEvent.isEmpty()) {
        const QJsonArray &dates = mEvent[STATE_EVENT_DATES].toArray();
        for (auto&& d : dates) {
            const QJsonObject &date = d.toObject();

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

                    item->setText(d.getQStringName());
                    item->setData(0x0101, d.getQStringName());
                    item->setData(0x0102, d.mPlugin->getId());
                    item->setData(0x0103, d.getDesc());
                    item->setData(0x0104, d.mId);
                    item->setData(0x0105, d.getWiggleDesc());
                    item->setData(0x0106, MHVariable::getSamplerProposalText(d.mTi.mSamplerProposal));
                    item->setData(0x0107, d.mIsValid);
                    item->setData(0x0108, date.value(STATE_DATE_SUB_DATES).toArray().size() > 0);
                    item->setData(0x0109, d.mOrigin);
                    item->setData(0x0110, QString::fromStdString(d.mUUID));
                    
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
    const QJsonArray &dates = mEvent[STATE_EVENT_DATES].toArray();
    if (index < dates.size()) {
        emit indexChange(index);
        emit calibRequested(dates[index].toObject());
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
    MainWindow::getInstance()->updateEvent(event, Project::ReasonId::DateUpdated);
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
