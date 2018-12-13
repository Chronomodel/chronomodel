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

    void setFont(const QFont& font) { mFont = font; update();}
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
