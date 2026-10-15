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

#ifndef ARROWTMPITEM_H
#define ARROWTMPITEM_H

#include <QGraphicsItem>

class ArrowTmpItem: public QGraphicsItem
{
public:
    enum State{
        eNormal,
        eAllowed,
        eForbidden
    };

    ArrowTmpItem(QGraphicsItem* parent = nullptr);

    void setFrom(const int& x, const int& y);
    void setTo(const int &x, const int &y);
    void setState(const State state);
    void setLocked(bool locked);

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

protected:

    QString getBubbleText() const;

    qreal mBubbleHeight;

    int mXFrom;
    int mYFrom;
    int mXTo;
    int mYTo;
    State mState;
    bool mLocked;
};

#endif
