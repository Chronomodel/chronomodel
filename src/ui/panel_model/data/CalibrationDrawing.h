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

#ifndef CALIBRATIONDRAWING_H
#define CALIBRATIONDRAWING_H

#include <QWidget>
#include <QLabel>
class GraphView;
class GraphViewRefAbstract;
class Marker;

class CalibrationDrawing : public QWidget
{
    Q_OBJECT

public slots:
    virtual void setVisible(bool visible);

public:
    explicit CalibrationDrawing(QWidget *parent = nullptr);
    ~CalibrationDrawing();

    void setTitle(const QString& title) { mTitle->setText(title);}
    void setRefTitle(const QString& title) { mRefTitle->setText(title);}
    void setRefComment(const QString& comment) { mRefComment->setText(comment);}
    void setRefGraph(GraphViewRefAbstract *refGraph);

    void setCalibTitle(const QString& title) { mCalibTitle->setText(title);}
    void setCalibComment(const QString& comment) { mCalibComment->setText(comment);}
    void setCalibGraph(GraphView *calibGraph);

    void setFont(const QFont& font) { mFont = font; }//update();}
    void hideMarker();
    void showMarker();

    void setMouseTracking(bool enable);

    virtual QFont font() const {return mFont;}
    void updateLayout();

private:
    QLabel* mTitle;
    QLabel* mRefTitle;
    QLabel* mRefComment;
    GraphViewRefAbstract* mRefGraphView;

    QLabel* mCalibTitle;
    QLabel* mCalibComment;
    GraphView* mCalibGraph;

    Marker* mMarkerX;
    Marker* mMarkerY;

    int mVerticalSpacer;
    QFont mFont;

protected:
    void paintEvent(QPaintEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* e);


    bool mMouseOverCurve;


};

#endif // CALIBRATIONDRAWING_H
