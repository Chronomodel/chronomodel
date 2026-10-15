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

#ifndef DATEDIALOG_H
#define DATEDIALOG_H

#include <QDialog>
#include <QJsonObject>
#include <QtGui/qvalidator.h>
#include <QtWidgets/qlineedit.h>

#include "version.h"
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
class Button;
class QLabel;
class HelpWidget;

// ---------------------------------------------------------------------
//  WiggleValidator.h
// ---------------------------------------------------------------------

class WiggleValidator : public QValidator
{
    Q_OBJECT
public:
    explicit WiggleValidator(QObject *parent = nullptr);

    /*  Les radios et les edits sont passés après construction (dans
        DateDialog::DateDialog) afin d’éviter les dépendances circulaires.   */
    void setRadios(QRadioButton *fixed,
                   QRadioButton *range,
                   QRadioButton *gauss);
    void setEdits(QLineEdit *fixedEdit,
                  QLineEdit *minEdit,
                  QLineEdit *maxEdit,
                  QLineEdit *avgEdit,
                  QLineEdit *errEdit);

    /*  Méthode obligatoire de QValidator  */
    State validate(QString &input, int &pos) const override;

    /*  Retourne le même résultat que la méthode validate() mais
        sans avoir besoin du paramètre « input » (pratique dans
        checkWiggle()).                                            */
    bool isWiggleValid() const;

private:

    // pointeurs vers les contrôles (non‑possédés)
    QRadioButton *mFixedRadio  = nullptr;
    QRadioButton *mRangeRadio  = nullptr;
    QRadioButton *mGaussRadio  = nullptr;

    QLineEdit    *mFixedEdit   = nullptr;
    QLineEdit    *mMinEdit     = nullptr;
    QLineEdit    *mMaxEdit     = nullptr;
    QLineEdit    *mAvgEdit     = nullptr;
    QLineEdit    *mErrEdit     = nullptr;

    mutable bool mLastResult = true;   // mémorise le dernier état (pour isWiggleValid())
};


class DateDialog: public QDialog
{
    Q_OBJECT
public:
    DateDialog(QWidget* parent = nullptr, Qt::WindowFlags flags =  Qt::WindowType::Widget);
    ~DateDialog();

    void setDate(const QJsonObject& date);
    void setForm(PluginFormAbstract* form);


#ifndef FIXEDPRIOR
    void setDataMethod(SamplerProposal sp);
    SamplerProposal getMethod() const;
#endif

    Date::DeltaType getDeltaType() const;

    QString getName() const
    {
        return mNameEdit->text();
    }
    double getDeltaFixed() const
    {
        return sLocale.toDouble(mDeltaFixedEdit->text());
    }
    double getDeltaMin() const
    {
        return sLocale.toDouble(mDeltaMinEdit->text());
    }
    double getDeltaMax() const
    {
        return sLocale.toDouble(mDeltaMaxEdit->text());
    }
    double getDeltaAverage() const
    {
        return sLocale.toDouble(mDeltaAverageEdit->text());
    }
    double getDeltaError() const
    {
        return sLocale.toDouble(mDeltaErrorEdit->text());
    }
    void setWiggleEnabled(bool enabled);

protected:
    bool mWiggleIsValid;
    bool mPluginDataAreValid;

protected slots:
    void setAdvancedVisible(bool visible);
    void updateVisibleControls();
    void setOkEnabled();
    void setPluginDataValid(bool valid);
    void checkWiggle();

signals:
    void wiggleChange();

public:
    PluginFormAbstract* mForm;

    QLabel* mNameLab;
    QLineEdit* mNameEdit;

    Collapsible* mAdvanced;

    QCheckBox* mAdvancedCheck;
    QGroupBox* mAdvancedWidget;

#ifndef FIXEDPRIOR
    QLabel* mMethodLab;
    QComboBox* mMethodCombo;
#endif

    QLabel* mWiggleLab;
    QRadioButton* mDeltaNoneRadio;
    QRadioButton* mDeltaFixedRadio;
    QRadioButton* mDeltaRangeRadio;
    QRadioButton* mDeltaGaussRadio;

    HelpWidget* mDeltaHelp;
    QLabel* mDeltaFixedLab;
    QLabel* mDeltaMinLab;
    QLabel* mDeltaMaxLab;
    QLabel* mDeltaAverageLab;
    QLabel* mDeltaErrorLab;

    QLineEdit* mDeltaFixedEdit;
    QLineEdit* mDeltaMinEdit;
    QLineEdit* mDeltaMaxEdit;
    QLineEdit* mDeltaAverageEdit;
    QLineEdit* mDeltaErrorEdit;

    int mWidth;
    int mMargin;
    int mLineH;
    int mButW;
    int mButH;
    int mComboH;
private:
    static QLocale sLocale;
    QDoubleValidator *mValidator_R;

    bool mWiggleEnabled;

    QVBoxLayout* mLayout;
    QDialogButtonBox* mButtonBox;
};




#endif
