/* ---------------------------------------------------------------------
Copyright or © or Copr. CNRS	2014 - 2024

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

#ifndef REBUILDCURVEDIALOG_H
#define REBUILDCURVEDIALOG_H

#include "LineEdit.h"
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDialog>
#include <QSpinBox>
#include <QComboBox>
#include <QtWidgets/qgridlayout.h>

class RebuildCurveDialog : public QDialog
{
    Q_OBJECT
public:
#ifdef DEBUG
    explicit RebuildCurveDialog (QStringList list = {"X"}, std::vector< std::pair<double, double>> *minMax = nullptr, std::vector< std::pair<double, double>> *minMaxP = nullptr, std::pair<double, double> *minMaxPFilter = nullptr, std::pair<unsigned, unsigned> mapSize = std::pair<unsigned, unsigned>{300, 300}, QWidget *parent = nullptr);
#else
    explicit RebuildCurveDialog (QStringList list = {"X"}, std::vector< std::pair<double, double>> *minMax = nullptr, std::vector< std::pair<double, double>> *minMaxP = nullptr, std::pair<unsigned, unsigned> mapSize = std::pair<unsigned, unsigned>{300, 300}, QWidget *parent = nullptr);
#endif
    std::vector< std::pair<double, double>> getYTabMinMax() const {return mYTabMinMax;}
    std::vector< std::pair<double, double>> getYpTabMinMax() const {return mYpTabMinMax;}
 #ifdef DEBUG
    std::pair<double, double> getYpMinMaxFilter() const {return mYpMinMaxFilter;}
#endif
    std::pair<unsigned, unsigned> getMapSize() const;

    int getXSpinResult() const;
    int getYSpinResult() const;
    int getYpSpinResult() const;

    void setCompoList (QStringList &list);

protected slots:
    void updateOptions();
    void Y1MinIsValid (QString str);
    void Y1MaxIsValid(QString str);

    void Y2MinIsValid(QString str);
    void Y2MaxIsValid(QString str);

    void Y3MinIsValid(QString str);
    void Y3MaxIsValid(QString str);

    void Y1pMinIsValid (QString str);
    void Y1pMaxIsValid(QString str);

#ifdef DEBUG
    void Y1pMinFilterIsValid (QString str);
    void Y1pMaxFilterIsValid(QString str);
#endif

    void Y2pMinIsValid(QString str);
    void Y2pMaxIsValid(QString str);

    void Y3pMinIsValid(QString str);
    void Y3pMaxIsValid(QString str);

protected slots:
    void setOkEnabled();

private:
    QLabel* label;
    QLineEdit* lineEdit;
    QDialogButtonBox* buttonBox;

    LineEdit* XStepEdit;
    LineEdit* YStepEdit;

    QStringList mCompoList;

    // 3 composantes possibles
    QLineEdit* Y1minEdit;
    bool Y1MinOK = true;
    QLineEdit *Y1maxEdit;
    bool Y1MaxOK = true;

    QLineEdit* Y2minEdit;
    bool Y2MinOK = true;
    QLineEdit *Y2maxEdit;
    bool Y2MaxOK = true;

    QLineEdit* Y3minEdit;
    bool Y3MinOK = true;
    QLineEdit *Y3maxEdit;
    bool Y3MaxOK = true;

    // 3 derrivées possibles
    QLineEdit* Y1pminEdit;
    bool Y1pMinOK = true;
    QLineEdit* Y1pmaxEdit;
    bool Y1pMaxOK = true;

    QLineEdit* Y2pminEdit;
    bool Y2pMinOK = true;
    QLineEdit* Y2pmaxEdit;
    bool Y2pMaxOK = true;

    QLineEdit* Y3pminEdit;
    bool Y3pMinOK = true;
    QLineEdit *Y3pmaxEdit;
    bool Y3pMaxOK = true;  

    std::vector< std::pair<double, double>> mYTabMinMax;
    std::vector< std::pair<double, double>> mYpTabMinMax;

    // 1 filter possible
#ifdef DEBUG
    QLineEdit* Y1pminFilterEdit;
    bool Y1pMinFilterOK = true;
    QLineEdit* Y1pmaxFilterEdit;
    bool Y1pMaxFilterOK = true;

    std::pair<double, double> mYpMinMaxFilter;
#endif

    QGridLayout* _setting_1_Grid;
    QGridLayout* _setting_2_Grid;
    QGridLayout* _setting_3_Grid;

signals:
    void OkEnabled(bool enabled = true) ;
};

#endif // REBUILDCURVEDIALOG_H
