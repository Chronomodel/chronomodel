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

#ifndef EVENTPROPERTIESVIEW_H
#define EVENTPROPERTIESVIEW_H

#include "Button.h"
#include "version.h"

#include <QCheckBox>
#include <QWidget>
#include <QJsonObject>

class Label;
class LineEdit;

class QLabel;
class QLineEdit;
class QComboBox;
class QGroupBox;
class QRadioButton;

class ColorPicker;
class DatesList;

class RadioButton;
class GraphView;

class CurveWidget;
class EventItem;


class EventPropertiesView: public QWidget
{
    Q_OBJECT
public:
    EventPropertiesView(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~EventPropertiesView();

    void updateEvent();
    const QJsonObject& getEvent();

    inline void setCalibChecked(bool checked) { mCalibBut->setChecked(checked);}
    inline bool isCalibChecked() const {return mCalibBut->isChecked();};
    bool hasEvent() const;
    bool hasBound() const;
    bool hasEventWithDates() const;
    void initEvent(const QJsonObject &eventObj);
    void resetEvent();

public slots:
    void setEvent(const QJsonObject &eventObj);
    void applyAppSettings();

protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void keyPressEvent(QKeyEvent* e);
    void updateLayout();
    QString XerrorLabel() const;

private slots:

    void updateEventName();
    void updateEventColor(const QColor& color);
    void updateEventSampler(int index);
    void updateIndex(int index);

    // Curve
    void updateCurveNode(bool isNode);
    void updateEventXInc();
    void updateEventYDec();
    void updateEventZF();


    void updateEventSXInc();
    void updateEventSYDec();
    void updateEventSZF();

    void createDate();
    void deleteSelectedDates();
    void recycleDates();

    void updateCombineAvailability();
    void sendCombineSelectedDates();
    void sendSplitDate();

    void updateButton();

    void updateKnownFixed(const QString& text);

    void updateKnownGraph();

signals:
    void combineDatesRequested(const int eventId, const QList<int>& dateIds);
    void splitDateRequested(const int eventId, const int dateId);

    void updateCalibRequested(const QJsonObject& date);
    void showCalibRequested(bool show);

private:
    int minimumHeight;
    QJsonObject mEventObj;
    int mCurrentDateIdx;

    QWidget* mTopView;
    QWidget* mEventView;
    QWidget* mBoundView;

    QLabel* mNameLab;

    QLabel* mColorLab;
    QLineEdit* mNameEdit;
    ColorPicker* mColorPicker;

#ifndef FIXEDPRIOR
    int mComboBoxHeight;

    QLabel* mMethodLab;
    QComboBox* mMethodCombo;
    QLabel* mMethodInfo;
#endif

    DatesList* mDatesList;
    QList<Button*> mPluginButs;

    Button* mDeleteBut;
    Button* mRecycleBut;

    Button* mCalibBut;
    Button* mCombineBut;
    Button* mSplitBut;

    QLineEdit* mKnownFixedEdit;

    GraphView* mKnownGraph;

    QGroupBox* mFixedGroup;

    int mButtonWidth;
    int mButtonHeigth;

    int mLineEditHeight;  

    CurveWidget* mCurveWidget;

    QCheckBox* mCurveNodeCB;

    QLabel* mX_IncLab;
    LineEdit* mX_IncEdit;

    QLabel* mS_X_IncLab;
    QLineEdit* mS_X_IncEdit;

    QLabel* mY_DecLab;
    LineEdit* mY_DecEdit;

    QLabel* mS_Y_Lab;
    QLineEdit* mS_Y_Edit;

    QLabel* mZ_IntLab;
    LineEdit* mZ_IntEdit;

    QLabel* mS_Z_IntLab;
    QLineEdit* mS_Z_IntEdit;

    bool mCurveEnabled;

};

#endif
