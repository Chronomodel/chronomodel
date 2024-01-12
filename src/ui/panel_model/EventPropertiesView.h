/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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

#ifndef EVENTPROPERTIESVIEW_H
#define EVENTPROPERTIESVIEW_H


#include <QCheckBox>
#include <QWidget>
#include <QJsonObject>

#include "Button.h"

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


class EventPropertiesView: public QWidget
{
    Q_OBJECT
public:
    EventPropertiesView(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~EventPropertiesView();

    void updateEvent();
    const QJsonObject& getEvent() const;

    inline void setCalibChecked(bool checked) { mCalibBut->setChecked(checked);}
    inline bool isCalibChecked() const {return mCalibBut->isChecked();};
    bool hasEvent() const;
    bool hasBound() const;
    bool hasEventWithDates() const;
    void initEvent(QJsonObject* event = nullptr);

public slots:
    void setEvent(QJsonObject *event);
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
    void updateEventMethod(int index);
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
    QJsonObject *mEvent;
    int mCurrentDateIdx;

    QWidget* mTopView;
    QWidget* mEventView;
    QWidget* mBoundView;

    QLabel* mNameLab;
    QLabel* mColorLab;
    QLabel* mMethodLab;

    QLineEdit* mNameEdit;
    ColorPicker* mColorPicker;
    QComboBox* mMethodCombo;
    QLabel* mMethodInfo;

    DatesList* mDatesList;
    QList<Button*> mPluginButs;
    //QList<std::unique_ptr<Button>> mPluginButs;

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

    int mComboBoxHeight;

    CurveWidget* mCurveWidget;

    QCheckBox* mCurveNodeCB;

    QLabel* mX_IncLab;
    QLineEdit* mX_IncEdit;

    QLabel* mS_X_IncLab;
    QLineEdit* mS_X_IncEdit;

    QLabel* mY_DecLab;
    QLineEdit* mY_DecEdit;

    QLabel* mS_Y_Lab;
    QLineEdit* mS_Y_Edit;

    QLabel* mZ_IntLab;
    QLineEdit* mZ_IntEdit;

    QLabel* mS_Z_IntLab;
    QLineEdit* mS_Z_IntEdit;

    bool mCurveEnabled;
   // CurveSettings::ProcessType mCurveProcessType;
};

#endif
