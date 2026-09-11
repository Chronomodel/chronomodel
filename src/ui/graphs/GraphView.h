/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

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

#pragma once

#include "GraphViewAbstract.h"
#include "GraphCurve.h"
#include "GraphZone.h"
#include "AxisTool.h"
#include "DateUtils.h"

#include <QWidget>
#include <QString>
#include <QFont>
#include <QColor>
#include <QPixmap>
#include <QFileInfo>
#include <QPainterPath>
#include <QStaticText>
#include <QPicture>
#include <QElapsedTimer>
#include <QTimer>
#include <QMap>
#include <QList>
#include <vector>

/*======================================================================
   CLASS GraphView
======================================================================*/
class GraphView : public GraphViewAbstract
{
    Q_OBJECT

public:

    enum class AxisMode : int {
        eHidden = 0,
        eMinMax = 1,
        eMinMaxHidden = 2,
        eMainTicksOnly = 3,
        eAllTicks = 4
    };

    /**  Flèches affichées lorsqu’une donnée déborde de l’axe X. */
    enum class OverflowDataArrowMode : int {
        eNone          = 0,   ///< aucune flèche
        eBothOverflow  = 1,   ///< flèches aux deux extrémités
        eUnderMin      = 2,   ///< flèche sous la valeur minimale
        eOverMax       = 3    ///< flèche au-dessus de la valeur maximale
    };


    /*------------------------------------------------------------------
       Constructeurs / destructeur
    ------------------------------------------------------------------*/
    explicit GraphView( QWidget* parent = nullptr );
    explicit GraphView( const GraphView& other, QWidget* parent = nullptr );
    ~GraphView() override;

    /*------------------------------------------------------------------
       Copie d’un GraphView existant
    ------------------------------------------------------------------*/
    void copyFrom( const GraphView& other );

    /*------------------------------------------------------------------
       Gestion des informations affichées (texte libre)
    ------------------------------------------------------------------*/
    inline void setInfo( const QString& info )
    {
        mInfos.clear();
        mInfos.append( info );
    }

    QString getInfo(char sep = '|' ) const { return mInfos.join(sep); }
    bool isShow() const { return mShowInfos; }


    inline void addInfo( const QString& info )   { mInfos << info; }
    inline void clearInfos()                     { mInfos.clear(); }
    inline void showInfos( bool show )           { mShowInfos = show; }

    /*------------------------------------------------------------------
       Couleur de fond & message « nothing »
    ------------------------------------------------------------------*/
    inline void   setBackgroundColor( const QColor& c ) { mBackgroundColor = c; }
    inline QColor backgroundColor()               const { return mBackgroundColor; }

    inline void setNothingMessage( const QString& msg ) { mNothingMessage = msg; }
    inline void resetNothingMessage()                  { mNothingMessage = tr( "Nothing to display" ); }

    /*------------------------------------------------------------------
       Paramètres d’affichage des axes (setters « no‑action »)
    ------------------------------------------------------------------*/
    inline void showXAxisLine( bool v )      { mXAxisLine      = v; }
    inline void showXAxisArrow( bool v )     { mXAxisArrow     = v; }
    inline void showXAxisTicks( bool v )     { mXAxisTicks     = v; }
    inline void showXAxisSubTicks( bool v )  { mXAxisSubTicks  = v; }
    inline void showXAxisValues( bool v )    { mXAxisValues    = v; }

    inline void showYAxisLine( bool v )      { mYAxisLine      = v; }
    inline void showYAxisArrow( bool v )     { mYAxisArrow     = v; }
    inline void showYAxisTicks( bool v )     { mYAxisTicks     = v; }
    inline void showYAxisSubTicks( bool v )  { mYAxisSubTicks  = v; }
    inline void showYAxisValues( bool v )    { mYAxisValues    = v; }

    /*------------------------------------------------------------------
       Modes d’axes
    ------------------------------------------------------------------*/
    inline void setXAxisMode( AxisMode mode )
    {
        mXAxisMode = mode;
        mAxisToolX.mShowText = (mode != AxisMode::eHidden);
    }
    void setYAxisMode( AxisMode mode );

    /*------------------------------------------------------------------
       Support d’axes (bottom / left / …)
    ------------------------------------------------------------------*/
    inline void setXAxisSupport( AxisTool::AxisSupport s ) { mAxisToolX.mSupport = s; }
    inline void setYAxisSupport( AxisTool::AxisSupport s ) { mAxisToolY.mSupport = s; }

    /*------------------------------------------------------------------
       Flèches de débordement
    ------------------------------------------------------------------*/

    inline void setOverArrow( OverflowDataArrowMode mode )
    {
        mOverflowArrowMode = mode;
    }

    /*------------------------------------------------------------------
       Ajustement automatique de l’échelle Y
    ------------------------------------------------------------------*/
    inline void autoAdjustYScale( bool active ) { mAutoAdjustYScale = active; }
    inline bool autoAdjustY() const               { return mAutoAdjustYScale; }
    void adjustYScale();

    /*------------------------------------------------------------------
       Fontes, épaisseur et opacité des courbes
    ------------------------------------------------------------------*/
    void setGraphFont( const QFont& f );

    inline void setCurvesThickness( int v ) { mThickness = v; }
    void updateCurvesThickness( int v );
    inline int  curvesThickness() const    { return mThickness; }

    inline void setCurvesOpacity( int v ) { mOpacity = v; }
    void updateCurvesOpacity( int v );

    inline void setCanControlOpacity( bool v ) { mCanControlOpacity = v; }

    /*------------------------------------------------------------------
       Gestion des courbes
    ------------------------------------------------------------------*/
    void add_curve( const GraphCurve& c );
    inline bool has_curves() const
    {
        return !mCurves.isEmpty() || !mZones.isEmpty();
    }

    void removeCurve( const QString& name );
    void removeAllCurves();
    void reserveCurves( int n );
    inline void squeezeCurves() { mCurves.squeeze(); }

    void setCurveVisible( const QString& name, bool visible );
    void setCurveVisible( const QStringList& names, bool visible );
    void setCurveVisible( std::initializer_list<const char*> names, bool visible );

    GraphCurve*               getCurve( const QString& name );
    const QList<GraphCurve>&  getCurves() const;
    int                       numCurves() const;

    /*------------------------------------------------------------------
       Points de référence (tool‑tips)
    ------------------------------------------------------------------*/
    inline bool has_points() const                     { return !refPoints.empty(); }
    inline CurveRefPts* get_refPoint( int i )          { return &refPoints[i]; }
    void set_points( const std::vector<CurveRefPts>& pts ) { refPoints = pts; }
    void add_point( const CurveRefPts& pt )                { refPoints.push_back( pt ); }
    inline void insert_points( const std::vector<CurveRefPts>& pts )
    {
        refPoints.insert( refPoints.end(), pts.begin(), pts.end() );
    }
    void set_points_visible( const QString& name, bool visible );

    /*------------------------------------------------------------------
       Zones (ex. zones de couleur sous le graphe)
    ------------------------------------------------------------------*/
    void add_zone( const GraphZone& z );

    /*------------------------------------------------------------------
       Fonctions de formatage des axes (conversion dates ↔ valeurs)
    ------------------------------------------------------------------*/
    inline void setFormatFunctX( DateConversion f ) { mUnitFunctionX = f; }
    inline void setFormatFunctY( DateConversion f ) { mUnitFunctionY = f; }

    /*------------------------------------------------------------------
       Division de l’échelle (major / minor)
    ------------------------------------------------------------------*/
    inline void setXScaleDivision( const Scale& s )               { mAxisToolX.setScaleDivision( s ); }
    inline void setXScaleDivision( double major, int minorCnt )   { mAxisToolX.setScaleDivision( major, minorCnt ); }

    inline void setYScaleDivision( const Scale& s )               { mAxisToolY.setScaleDivision( s ); }
    inline void setYScaleDivision( double major, int minorCnt )   { mAxisToolY.setScaleDivision( major, minorCnt ); }

    /*------------------------------------------------------------------
       Export / sauvegarde
    ------------------------------------------------------------------*/
    bool saveAsSVG( const QString& fileName,
                   const QString& svgTitle,
                   const QString& svgDescription,
                   bool withVersion,
                   int versionHeight = 20 );

    /*------------------------------------------------------------------
       Tool‑tips
    ------------------------------------------------------------------*/
    void setTipXLab( const QString& lab );
    void setTipYLab( const QString& lab );

    /*------------------------------------------------------------------
       Méthodes graphique
    ------------------------------------------------------------------*/
    /** Dessine le graphe sur n’importe quel QPaintDevice (PDF, image, …). */
    void paintToDevice( QPaintDevice* device );

    /** Forçage d’un rafraîchissement immédiat. */
    inline void forceRefresh()
    {
        repaintGraph();
    }

public slots:
    void zoomX( type_data min, type_data max );

    void exportCurrentDensities( const QString& defaultPath,
                                const QLocale& locale,
                                const QString& csvSep,
                                double step = 1.0 ) const;

    void exportCurrentVectorCurves( const QString& defaultPath,
                                   const QLocale& locale,
                                   const QString& csvSep,
                                   bool writeInRows,
                                   int offset = 0 ) const;

    void exportCurrentCurves( const QString& defaultPath,
                             const QLocale locale,
                             const QString& csvSep,
                             double step = 1.0,
                             QString graphTitle = "" ) const;

    void exportReferenceCurves( const QString& defaultPath,
                               const QLocale locale = QLocale::English,
                               const QString& csvSep = ",",
                               double step = 1.0,
                               QString fileName = "",
                               double threshold = 95.0,
                               bool isHPD = false ) const;

    void changeXScaleDivision( const Scale& sc );
    void changeXScaleDivision( double major, int minor );


protected:
    /*------------------------------------------------------------------
       Méthodes de mise à jour et de dessin
    ------------------------------------------------------------------*/
    void adaptMarginBottom();
    void updateGraphSize( qreal w, qreal h );

    QPainterPath makePath( const QMap<double,double>& map, bool showBorder ) const;
    void         drawCurves( QPainter& p );
    void         drawMap( const GraphCurve& c, QPainter& p );
    QPainterPath makeSubShape( const QMap<double,double>& inf,
                              const QMap<double,double>& sup ) const;
    void         drawShape( const GraphCurve& c, QPainter& p );
    void         drawDensity( const GraphCurve& c, QPainter& p );

    /*------------------------------------------------------------------
       Événements Qt
    ------------------------------------------------------------------*/
    void resizeEvent( QResizeEvent* ) override;
    void paintEvent( QPaintEvent* )   override;
    void repaintGraph()               override;
    void updateRasterCache();
    void drawTooltip();

    void enterEvent( QEnterEvent* ) override;
    void leaveEvent( QEvent* )      override;
    void mouseMoveEvent( QMouseEvent* ) override;

    void autoUpdate();   // mise à jour automatique via timer


protected:   // ---------- membres protégés ----------
    AxisTool mAxisToolX;
    AxisTool mAxisToolY;
    qreal    mStepMinWidth = 0.0;

    // visibilité des éléments d’axes
    bool mXAxisLine      = true;
    bool mXAxisArrow     = true;
    bool mXAxisTicks     = true;
    bool mXAxisSubTicks  = false;
    bool mXAxisValues    = true;

    bool mYAxisLine      = true;
    bool mYAxisArrow     = true;
    bool mYAxisTicks     = true;
    bool mYAxisSubTicks  = false;
    bool mYAxisValues    = true;

    AxisMode               mXAxisMode = AxisMode::eAllTicks;
    AxisMode               mYAxisMode = AxisMode::eAllTicks;
    OverflowDataArrowMode  mOverflowArrowMode = OverflowDataArrowMode::eNone;

    // caches graphiques
    QPicture mVectorCache;   // qualité vectorielle
    QPixmap  mRasterCache;   // performance raster

    QElapsedTimer mLastFastPaint;
    bool          mIsScrolling = false;
    bool          mCacheValid  = false;
    QTimer*       mUpdateTimer = nullptr;   // mise à jour automatique

    bool mAutoAdjustYScale = false;

    // informations affichées sous le graphe
    bool        mShowInfos = false;
    QStringList mInfos;

    QString mNothingMessage;

    // apparence générale
    QColor mBackgroundColor = Qt::white;
    int    mThickness       = 1;
    int    mOpacity         = 255;
    bool   mCanControlOpacity = false;

    // tool‑tip
    QRectF  mTipRect;
    qreal   mTipX = 0.0, mTipY = 0.0;
    QString mTipXLab, mTipYLab, mTipComment;
    qreal   mTipWidth = 0.0, mTipHeight = 0.0;
    bool    mTipVisible = false;
    bool    mUseTip = false;

    // données graphiques
    QList<GraphCurve> mCurves;
    QList<GraphZone>  mZones;

    QPainter mPrevPainter;   // utilisé pour le cache raster

    qreal mBottomSpacer = 0.0;

public:   // ---------- membres publics ----------
    QString mLegendX;
    QString mLegendY;
    std::vector<CurveRefPts> refPoints;

private:   // ---------- membres privés ----------
    DateConversion mUnitFunctionX = nullptr;
    DateConversion mUnitFunctionY = nullptr;
};

/*======================================================================
   CLASS GraphTitle – titre et sous‑titre du graphe
======================================================================*/
class GraphTitle : public GraphViewAbstract
{
    Q_OBJECT

protected:
    qreal        mTitleHeight = 0.0;
    qreal        mSubTitleHeight = 0.0;

    QStaticText  mTitle;
    QStaticText  mCommentTitle;
    QStaticText  mSubTitle;

    QColor       mBackgroundColor = Qt::white;
    QColor       mTitleBarColor   = Qt::gray;

    bool mAutoAdjustTitleHeight    = true;
    bool mAutoAdjustSubTitleHeight = true;

public:
    explicit GraphTitle( QWidget* parent = nullptr );
    ~GraphTitle() override = default;

    explicit GraphTitle( const QString& title, QWidget* parent = nullptr );
    explicit GraphTitle( const QString& title, const QColor& titleBarColor, QWidget* parent = nullptr );

    explicit GraphTitle( const QString& title, const QString& subTitle, QWidget* parent = nullptr );
    explicit GraphTitle( const QString& title, const QString& subTitle,
                        const QColor& backGround, QWidget* parent = nullptr );

    explicit GraphTitle( const QString& title, const QString& commentTitle,
                        const QString& subTitle, QWidget* parent = nullptr );

    void paintEvent( QPaintEvent* ) override;
    void repaintGraph() override;

    // -----------------------------------------------------------------
    //   Setters / getters
    // -----------------------------------------------------------------
    inline void setTitle( const QString& t )          { mTitle.setText( t ); }
    inline void setSubTitle( const QString& t )       { mSubTitle.setText( t ); }
    inline void setBackGroundColor( const QColor& c)  { mBackgroundColor = c; }
    inline void setTitleBarColor( const QColor& c )   { mTitleBarColor = c; }

    inline void setTitleHeight( qreal h )
    {
        mTitleHeight = h;
        mAutoAdjustTitleHeight = false;
    }
    inline qreal titleHeight() const { return mTitleHeight; }

    inline void setSubTitleHeight( qreal h )
    {
        mSubTitleHeight = h;
        mAutoAdjustSubTitleHeight = false;
    }
    inline qreal subTitleHeight() const { return mSubTitleHeight; }

    inline void setAutoAdjustTitleHeight( bool a )    { mAutoAdjustTitleHeight = a; }
    inline void setAutoAdjustSubTitleHeight( bool a ) { mAutoAdjustSubTitleHeight = a; }
    inline bool autoAdjustTitleHeight()   const { return mAutoAdjustTitleHeight; }
    inline bool autoAdjustSubTitleHeight() const { return mAutoAdjustSubTitleHeight; }

    qreal height() const;

    inline bool isTitle()   const { return !mTitle.text().isEmpty(); }
    inline bool withTitle() const { return !mSubTitle.text().isEmpty(); }
};

