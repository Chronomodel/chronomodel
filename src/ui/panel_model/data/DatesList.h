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

#ifndef DATESLIST_H
#define DATESLIST_H

#include <QListWidget>
#include <QJsonObject>

class QListWidgetItem;
class Event;
class Date;


class DatesList: public QListWidget
{
    Q_OBJECT
public:
    DatesList(QWidget* parent = nullptr);
    ~DatesList();

    void setEvent(const QJsonObject event);

protected slots:
    void handleItemClicked(QListWidgetItem* item);
    void handleItemDoubleClicked(QListWidgetItem* item);
    void forceAtLeastOneSelected();

signals:
    void calibRequested(const QJsonObject& date);
    void indexChange(const int idx);

protected:
    void dropEvent(QDropEvent* e);
    void keyPressEvent(QKeyEvent* e);

private:
    QJsonObject mEvent;
    QList<QListWidgetItem*> mSelectedItems;
    bool mUpdatingSelection;
};

#endif
