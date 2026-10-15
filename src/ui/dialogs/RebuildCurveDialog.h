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

#ifndef REBUILDCURVEDIALOG_H
#define REBUILDCURVEDIALOG_H

#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDialog>
#include <QSpinBox>
#include <QComboBox>
#include <QtWidgets/qgridlayout.h>

#define WITHCURVEFILTER


class RebuildCurveDialog : public QDialog
{
    Q_OBJECT
public:
#ifdef WITHCURVEFILTER
    explicit RebuildCurveDialog (QStringList list = {"X"}, std::vector< std::pair<double, double>> *minMax = nullptr, std::vector< std::pair<double, double>> *minMaxP = nullptr, std::pair<double, double> *minMaxPFilter = nullptr, std::pair<unsigned, unsigned> mapSize = std::pair<unsigned, unsigned>{300, 300}, QWidget *parent = nullptr);
#else
    explicit RebuildCurveDialog (QStringList list = {"X"}, std::vector< std::pair<double, double>> *minMax = nullptr, std::vector< std::pair<double, double>> *minMaxP = nullptr, std::pair<unsigned, unsigned> mapSize = std::pair<unsigned, unsigned>{300, 300}, QWidget *parent = nullptr);
#endif
    std::vector< std::pair<double, double>> getYTabMinMax() const {return mYTabMinMax;}
    std::vector< std::pair<double, double>> getYpTabMinMax() const {return mYpTabMinMax;}

#ifdef WITHCURVEFILTER
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

#ifdef WITHCURVEFILTER
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

    QLineEdit* XStepEdit;
    QLineEdit* YStepEdit;

    QStringList mCompoList;

    // 3 composantes possibles
    QLineEdit* Y1minEdit;
    bool Y1MinOK = true;
    QLineEdit* Y1maxEdit;
    bool Y1MaxOK = true;

    QLineEdit* Y2minEdit;
    bool Y2MinOK = true;
    QLineEdit* Y2maxEdit;
    bool Y2MaxOK = true;

    QLineEdit* Y3minEdit;
    bool Y3MinOK = true;
    QLineEdit* Y3maxEdit;
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
#ifdef WITHCURVEFILTER
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
