/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2026

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
