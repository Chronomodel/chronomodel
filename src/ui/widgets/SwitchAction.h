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

#ifndef SWITCHACTION_H
#define SWITCHACTION_H

#include <QWidgetAction>
#include <QWidget>
#include <QComboBox>
#include <QLabel>

class SwitchWidget: public QWidget
{
    Q_OBJECT
public:
    SwitchWidget(QWidget* parent);
    SwitchWidget(QWidget* parent, QWidgetAction* action);

    void setToggled(bool toggled);

protected:
    void paintEvent(QPaintEvent* event);
    void mousePressEvent(QMouseEvent* event);
    
signals:
    void toggled(bool toggled);

public slots:
    void setText (QString str) { mText = str; repaint();};

protected:
    QWidgetAction* mAction;
    QString mText;
    bool mToggled;
};


class SwitchAction: public QWidgetAction
{
    Q_OBJECT
public:
    SwitchAction(QObject* parent);

protected:
    QWidget* createWidget(QWidget* parent);
};

#endif
