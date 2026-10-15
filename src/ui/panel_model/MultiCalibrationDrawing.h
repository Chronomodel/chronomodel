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

#ifndef MULTICALIBRATIONDRAWING_H
#define MULTICALIBRATIONDRAWING_H

#include "GraphView.h"
#include "Marker.h"

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QScrollArea>

class ColoredBar: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorsChanged)

public:
    ColoredBar(QWidget* parent = nullptr);
    ~ColoredBar();
    void setColor(QColor &color) {mColor = color;}
    QColor color() const {return mColor;}
    static int mWidth;

signals:
    void colorsChanged(QColor color);

protected:
    void paintEvent(QPaintEvent* e);

private:
    QColor mColor;

};

class MultiCalibrationDrawing: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int graphHeight READ graphHeight WRITE setGraphHeight NOTIFY graphHeightChanged)

public:
    MultiCalibrationDrawing(QWidget *parent = nullptr);
    ~MultiCalibrationDrawing();
    virtual QPixmap grab();


    void setGraphList(QList<GraphViewAbstract*> &list);
    void setEventsColorList(QList<QColor> &colorList);

    QList<GraphViewAbstract*> *getGraphList() {return &mListCalibGraph;}
    QList<GraphView*> getGraphViewList() const ;

    inline void setListAxisVisible(QList<bool> &list) { mListAxisVisible = list;};

    void updateLayout();
    //void forceRefresh();

    inline qreal graphHeight() const {return mGraphHeight;}
    void setGraphHeight(int height);
    inline void setGraphHeightForVisibleAxis (qreal height)
    {
        mHeightForVisibleTicksAxis = height;
    };
    void hideMarker();
    void showMarker();

    inline QWidget* getGraphWidget() const {return mGraphWidget;}

signals:
    void graphHeightChanged(int height);

private:

    QList<GraphViewAbstract*> mListCalibGraph;

    QList<QColor> mListEventsColor;
    QList<ColoredBar*> mListBar;
    QList<bool> mListAxisVisible;

    QScrollArea* mScrollArea;
    QWidget* mGraphWidget;

    Marker* mMarkerX;

    int mVerticalSpacer;

    qreal mGraphHeight;
    qreal mHeightForVisibleTicksAxis;

    QFont mGraphFont;

    bool mMouseOverCurve;


protected:
    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* e);

};

#endif // MULTICALIBRATIONDRAWING_H
