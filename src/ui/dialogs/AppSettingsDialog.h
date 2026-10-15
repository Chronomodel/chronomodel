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

#ifndef APPSETTINGSDIALOG_H
#define APPSETTINGSDIALOG_H

#include <QDialog>
#include <Painting.h>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QComboBox;
class QAbstractButton;
class QDialogButtonBox;
class QListWidget;
class QStackedWidget;

class CheckBox;
class Label;
class LineEdit;
class Button;

class AppSettingsDialog: public QDialog
{
    Q_OBJECT
public:
    AppSettingsDialog(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    virtual ~AppSettingsDialog();

    void setSettings();
    void getSettings();

    bool filesChanged;
    ColorPalette mMapPalette;

private slots:
    void changeSettings();
    void buttonClicked(QAbstractButton*);
    void needCalibration();
    void restore();

signals:
    void settingsChanged();
    void settingsFilesChanged();

private:
    QListWidget* mList;
    QStackedWidget* mStack;

    QWidget* mGeneralView;

    QLabel* mLangHelpLab;

    QLabel* mLanguageLab;
    QComboBox* mLanguageCombo;

    //Label* mFontLab;
    //Button* mFontBut;
    //QFont mFont;

   // QLabel* mCountryLab;
   // QComboBox* mCountryCombo;
    QLabel *mIconSizeLab;
    QSpinBox *mIconSize;

    QLabel* mMapPaletteLab;
    QComboBox* mMapPaletteCombo;


    QLabel* mAutoSaveLab;
    QCheckBox* mAutoSaveCheck;
    QLabel* mAutoSaveDelayLab;
    QLineEdit* mAutoSaveDelayEdit;

    QLabel* mCSVCellSepLab;
    QLineEdit* mCSVCellSepEdit;

    QLabel* mCSVDecSepLab;
    QComboBox* mCSVDecSepCombo;

    QLabel* mOpenLastProjectLab;
    QCheckBox* mOpenLastProjectCheck;

    QLabel* mPixelRatioLab;
    QSpinBox* mPixelRatio;

    /* disabled since v 1.6.4_alpha
     QLabel* mDpmLab;
     QComboBox* mDpm;
    */

    QLabel* mImageQualityLab;
    QSpinBox* mImageQuality;

    QLabel* mFormatDateLab;
    QComboBox* mFormatDate;

    QLineEdit* mCustomDateEdit;

    QLabel* mPrecisionLab;
    QSpinBox* mPrecision;

    QComboBox* mEventModelCombo;

    QDialogButtonBox* mApplyButton;
    QPushButton *mRestoreBox;
};

#endif
