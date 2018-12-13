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

#ifndef STUDYPERIODDIALOG_H
#define STUDYPERIODDIALOG_H

#include <QDialog>
#include <QJsonObject>
#include "Date.h"

class QLineEdit;
class QComboBox;
class QVBoxLayout;
class QGroupBox;
class QCheckBox;
class QDialogButtonBox;

class PluginFormAbstract;
class Collapsible;
class QRadioButton;
class QDoubleSpinBox;
class QLabel;
class HelpWidget;


class StudyPeriodDialog: public QDialog
{
    Q_OBJECT
public:
    StudyPeriodDialog(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Window);
    ~StudyPeriodDialog();

    void setStep(double step, bool forced, double suggested);
    void setSettings(const ProjectSettings& s);
    ProjectSettings getSettings() const;
    double step() const;
    bool forced() const;

protected slots:
    void setAdvancedVisible(bool visible);
    void updateVisibleControls();
    void setOkEnabled(const QString& text);

signals:
    void wiggleChange();

public:
    QLabel* mMinLab;
    QLabel* mMaxLab;

    QLineEdit* mMinEdit;
    QLineEdit* mMaxEdit;

    Collapsible* mAdvanced;

    QCheckBox* mAdvancedCheck;
    QGroupBox* mAdvancedWidget;

    QLabel* mForcedLab;
    QCheckBox* mForcedCheck;
    QLabel* mStepLab;

    QDoubleSpinBox* mStepSpin;

    int mMargin;
    int mLineH;
    int mButW;
    int mButH;
    int mComboH;

    QVBoxLayout* mLayout;
    QDialogButtonBox* mButtonBox;
};

#endif
