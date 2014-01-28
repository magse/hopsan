/*-----------------------------------------------------------------------------
 This source file is part of Hopsan NG

 Copyright (c) 2011
    Mikael Axin, Robert Braun, Alessandro Dell'Amico, Björn Eriksson,
    Peter Nordin, Karl Pettersson, Petter Krus, Ingo Staack

 This file is provided "as is", with no guarantee or warranty for the
 functionality or reliability of the contents. All contents in this file is
 the original work of the copyright holders at the Division of Fluid and
 Mechatronic Systems (Flumes) at Linköping University. Modifying, using or
 redistributing any part of this file is prohibited without explicit
 permission from the copyright holders.
-----------------------------------------------------------------------------*/

//!
//! @file   PlutCurve.cpp
//! @author Robert Braun <robert.braun@liu.se>
//! @date   2010
//!
//! @brief Contains a class for plot curves
//!
//$Id: ModelHandler.cpp 5551 2013-06-20 08:54:16Z petno25 $

//Hopsan includes
#include "global.h"
#include "Configuration.h"
#include "LogDataHandler.h"
#include "ModelHandler.h"
#include "PlotCurve.h"
#include "PlotTab.h"
#include "PlotArea.h"
#include "PlotWindow.h"
#include "Widgets/ModelWidget.h"
#include "Widgets/ProjectTabWidget.h"
#include "Utilities/GUIUtilities.h"

//Other includes
#include <limits>
#include <qwt_plot_zoomer.h>

const double DoubleMax = std::numeric_limits<double>::max();


class AlignmentSelectionStruct
{
public:
    AlignmentSelectionStruct(const Qt::Alignment alignment, const QString &label)
    {
        mAlignment = alignment;
        mLabel = label;
    }
    Qt::Alignment mAlignment;
    QString mLabel;
};



//! @brief Constructor for plot curves.
//! @param pData A shared pointer to the data to plot
//! @param curveType The type of the curve (controls the name and some other special things)
//! @todo why is the axis in the curve constructor, it would make more sence if the axis is specified when adding a curve to a plot area /Peter
PlotCurve::PlotCurve(SharedVariablePtrT pData, const QwtPlot::Axis axisY, const HopsanPlotCurveTypeEnumT curveType)
    : QObject(), QwtPlotCurve()
{
    mpParentPlotArea = 0;
    mHaveCustomData = false;
    mpData = pData;

    mLocalAdditionalCurveScale = 1.0;
    mLocalAdditionalCurveOffset = 0.0;
    mCustomCurveDataUnitScale = 1.0;
    mpCurveSymbol = 0;
    mCurveSymbolSize = 8;
    mIsActive = false;
    mIncludeGenInTitle = true;
    mIncludeSourceInTitle = false;
    mCurveType = curveType;

    mAxisY = axisY;
    mAutoUpdate = true;

    // Set QwtPlotCurve stuff
    //! @todo maybe this code should be run when we are adding a curve to a plottab
    refreshCurveTitle();
    updateCurve();
    this->setYAxis(axisY);
    this->setItemAttribute(QwtPlotItem::Legend, true);

    if(curveType != PortVariableType)
    {
        setAutoUpdate(false);
    }

    // Create relay connections
    connect(this, SIGNAL(curveDataUpdated()), this, SIGNAL(curveInfoUpdated()));
    // Create data connections
    connectDataSignals();

    if (mpData->getLogDataHandler())
    {
        mpData->getLogDataHandler()->incrementOpenPlotCurves();
    }
}

void PlotCurve::refreshCurveTitle()
{
    setTitle(getCurveName(mIncludeGenInTitle, mIncludeSourceInTitle));
}

//! @brief Destructor for plot curves
PlotCurve::~PlotCurve()
{
    // If the curve data had a data handler then decrement its open curves counter
    LogDataHandler* pDataHandler = mpData->getLogDataHandler();
    if (pDataHandler)
    {
        pDataHandler->decrementOpenPlotCurves();
    }

    // Delete custom data if any
    deleteCustomData();
}

void PlotCurve::setIncludeGenerationInTitle(bool doit)
{
    mIncludeGenInTitle=doit;
}

void PlotCurve::setIncludeSourceInTitle(bool doit)
{
    mIncludeSourceInTitle=doit;
}


//! @brief Returns the current generation a plot curve is representing
int PlotCurve::getGeneration() const
{
    return mpData->getGeneration();
}

QString PlotCurve::getCurveName() const
{
    if (mpData->hasCustomLabel())
    {
        return mpData->getCustomLabel();
    }
    else if (mpData->hasAliasName())
    {
        return mpData->getAliasName();
    }
    else
    {
        return mpData->getFullVariableNameWithSeparator(", ");
    }
}

QString PlotCurve::getCurveName(bool includeGeneration, bool includeSourceFile) const
{
    QString name = getCurveName();
    if (includeGeneration)
    {
        name.append(QString("  (%1)").arg(mpData->getGeneration()+1));
    }
    if (includeSourceFile)
    {
        QString source;
        if (mpData->isImported())
        {
            source = mpData->getImportedFileName();
        }
        else
        {
            source = mpData->getModelPath();
        }

        if (!source.isEmpty())
        {
            QFileInfo file(source);
            name.append(QString("    %1").arg(file.fileName()));
        }
    }
    return name;
}


//! @brief Returns the type of the curve
HopsanPlotCurveTypeEnumT PlotCurve::getCurveType()
{
    return mCurveType;
}


//! @brief Returns the name of the component a plot curve is created from
const QString &PlotCurve::getComponentName() const
{
    return mpData->getComponentName();
}


//! @brief Returns the name of the port a plot curve is created from
const QString &PlotCurve::getPortName() const
{
    return mpData->getPortName();
}


//! @brief Returns the data name (physical quantity) of a plot curve
const QString &PlotCurve::getDataName() const
{
    return mpData->getDataName();
}


//! @brief Returns the current custom data unit of a plot curve
const QString &PlotCurve::getDataCustomPlotUnit() const
{
    return mpData->getPlotScaleDataUnit();
}

//! @brief Returns the original data unit of a plot curve
const QString &PlotCurve::getDataOriginalUnit() const
{
    return mpData->getDataUnit();
}

//! @brief Returns the current unit of a plot curve in the following priority (Local unit, Data unit or Original unit)
const QString &PlotCurve::getCurrentUnit() const
{
    if (mCustomCurveDataUnit.isEmpty())
    {
        const QString &unit = getDataCustomPlotUnit();
        if (unit.isEmpty())
        {
            return getDataOriginalUnit();
        }
        else
        {
            return unit;
        }
    }
    else
    {
        return mCustomCurveDataUnit;
    }
}

VariableSourceTypeT PlotCurve::getDataSource() const
{
    return mpData->getVariableSourceType();
}

const QString &PlotCurve::getDataModelPath() const
{
    return mpData->getModelPath();
}


const SharedVariablePtrT PlotCurve::getDataVariable() const
{
    return mpData;
}


//! @brief Tells which Y-axis a plot curve is assigned to
int PlotCurve::getAxisY()
{
    return mAxisY;
}


//! @brief Returns the (unscaled) data vector of a plot curve
QVector<double> PlotCurve::getDataVectorCopy() const
{
    //! @todo this is no longer a reference need to see where it was used to avoid REALY slow code feetching data all the time /Peter
    return mpData->getDataVectorCopy();
}

//! @brief Returns the minimum and maximum value of the curve (for values higher then 0)
//! @details values <= 0 are ignored
bool PlotCurve::minMaxPositiveNonZeroYValues(double &rMin, double &rMax)
{
    int imax, imin;
    return mpData->positiveNonZeroMinMaxOfData(rMin, rMax, imin, imax);
}

bool PlotCurve::minMaxPositiveNonZeroXValues(double &rMin, double &rMax)
{
    int imax, imin;
    if (!mpCustomXdata.isNull())
    {
        return mpCustomXdata->positiveNonZeroMinMaxOfData(rMin, rMax, imin, imax);
    }
    else if (!mpData->getSharedTimeOrFrequencyVector().isNull())
    {
        return mpData->getSharedTimeOrFrequencyVector()->positiveNonZeroMinMaxOfData(rMin, rMax, imin, imax);
    }
    else
    {
        rMin = 0;
        rMax = mpData->getDataSize()-1;
        return (rMax > -1);
    }
}


//! @brief Returns the shared time or frequency vector of the plot curve
//! This returns the TIME vector, NOT any special X-axes if they are used.
const SharedVariablePtrT PlotCurve::getSharedTimeOrFrequencyVariable() const
{
    return mpData->getSharedTimeOrFrequencyVector();
}

bool PlotCurve::hasCustomXVariable() const
{
    return !mpCustomXdata.isNull();
}

const SharedVariablePtrT PlotCurve::getSharedCustomXVariable() const
{
    return mpCustomXdata;
}


//! @brief Sets the generation of a plot curve
//! Updates the data to specified generation, and updates plot info box.
//! @param genereation Genereation to use
bool PlotCurve::setGeneration(const int generation)
{
    QPointer<LogVariableContainer> pContainer = mpData->getLogVariableContainer();
    // Make sure we don try to use container from a variable that does not have one (then we cant switch generation)
    if(pContainer)
    {
        //! @todo maybe not set generation if same as current but what aboput custom x-axis
        // Make sure we have the data requested
        SharedVariablePtrT pNewData = pContainer->getDataGeneration(generation);
        if (pNewData)
        {
            disconnectDataSignals();
            mpData = pNewData;
            connectDataSignals();
        }
        else
        {
            //! @todo we should actually check custom x data also to make sure that we have data that will be ok
            return false;
        }

        if (hasCustomXVariable())
        {
            QPointer<LogVariableContainer> pCustXContainer = mpCustomXdata->getLogVariableContainer();
            if (pCustXContainer)
            {
                SharedVariablePtrT pNewXData = pCustXContainer->getDataGeneration(generation);
                if (pNewXData)
                {
                    setCustomXData(pNewXData);
                }
            }
            //! @todo else what ??
        }

        updateCurve();
        refreshCurveTitle();

        //! @todo should this be done here
        mpParentPlotArea->resetZoom();

        return true;
    }
    return false;
}


//! @brief Sets the unit of a plot curve
//! @details The physical quantity will be checked, if it does not match the current unit, the new unit will be ignored
//! @param[in] rUnit Name of new unit
//! @note If unit is not registered for data then nothing will happen
void PlotCurve::setCustomCurveDataUnit(const QString &rUnit)
{
    // For non signal variables
    if (getDataName() != "Value")
    {
        // Check so that this unit is relevant for this type of data (datname). Else it will be ignored
        if (gpConfig->hasUnitScale(getDataName(),rUnit))
        {
            setCustomCurveDataUnit(rUnit, gpConfig->getUnitScale(getDataName(), rUnit));
        }
    }
    // For signal variables
    else
    {
        // Only set the new unit if it represents the same physical quantity as the current unit
        QStringList pqs = gpConfig->getPhysicalQuantitiesForUnit(rUnit);
        QStringList pqsOrg = gpConfig->getPhysicalQuantitiesForUnit(getDataOriginalUnit());
        if ( !(pqs.isEmpty() || pqsOrg.isEmpty()) )
        {
            if (pqs.front() == pqsOrg.front())
            {
                setCustomCurveDataUnit(rUnit, gpConfig->getUnitScale(pqs.first(), rUnit));
            }
        }
    }
}

//! @brief Sets a custom unit and scale of a plot curve
//! @param unit Name of new unit
//! @param scale What scaling towards default (usually SI) unit to use
void PlotCurve::setCustomCurveDataUnit(const QString &rUnit, double scale)
{
    mCustomCurveDataUnit = rUnit;
    mCustomCurveDataUnitScale = scale*1.0/mpData->getPlotScale();

    updateCurve();

    //! @todo shouldnt these be triggered by signal in update curve?
    mpParentPlotArea->replot();
}

void PlotCurve::removeCustomCurveDataUnit()
{
    mCustomCurveDataUnit.clear();
    mCustomCurveDataUnitScale = 1.0;

    updateCurve();

    //! @todo shouldnt these be triggered by signal in update curve?
    mpParentPlotArea->replot();
}


//! @brief Sets the (plot only) scaling of a plot curve
//! @param scaleX Scale factor for X-axis
//! @param scaleY Scale factor for Y-axis
//! @param offsetX Offset value for X-axis
//! @param offsetY Offset value for Y-axis
//! @todo FIXA /Peter
void PlotCurve::setTimePlotScalingAndOffset(double scale, double offset)
{
    mpData->setTimePlotScaleAndOffset(scale, offset);
}

void PlotCurve::setLocalCurvePlotScaleAndOffset(const double scale, const double offset)
{
    mLocalAdditionalCurveScale = scale;
    mLocalAdditionalCurveOffset = offset;
    updateCurve();
}

void PlotCurve::setDataPlotOffset(const double offset)
{
    mpData->setPlotOffset(offset);
    //The dataChanged signal is emitted inside setPlotOffset
}


void PlotCurve::setCustomData(const VariableDescription &rVarDesc, const QVector<double> &rvTime, const QVector<double> &rvData)
{
    // First disconnect all signals from the old data
    disconnectDataSignals();

    // If we already have custom data, then delete it from memory as it is being replaced
    deleteCustomData();

    // Create new custom data
    //! @todo we are abusing tiedomain variable here
    mpData = SharedVariablePtrT(new TimeDomainVariable(createFreeTimeVectorVariabel(rvTime), rvData, 0,
                                                       SharedVariableDescriptionT(new VariableDescription(rVarDesc)), SharedMultiDataVectorCacheT()));
    mHaveCustomData = true;

    // Connect signals
    connectDataSignals();

    updateCurve();
}

void PlotCurve::setCustomXData(const VariableDescription &rVarDesc, const QVector<double> &rvXdata)
{
    setCustomXData(createFreeVectorVariable(rvXdata, SharedVariableDescriptionT(new VariableDescription(rVarDesc))));
}

void PlotCurve::setCustomXData(SharedVariablePtrT pData)
{
    //! @todo maybe prevent reset if timevector is null, but then it will (currently) be impossible to reset x vector in curve.

    // Disconnect any signals first, in case we are changing x-data
    disconnectCustomXDataSignals();
    // Set new data and connect signals
    mpCustomXdata = pData;
    connectCustomXDataSignals();

    // Redraw curve
    updateCurve();
}

void PlotCurve::setCustomXData(const QString fullName)
{
    // If empty then reset time vector
    if (fullName.isEmpty())
    {
        setCustomXData(SharedVariablePtrT());
    }
    else
    {
        LogDataHandler *pHandler = mpData->getLogDataHandler();
        if (pHandler)
        {
            SharedVariablePtrT pData = pHandler->getLogVariableDataPtr(fullName, mpData->getGeneration());
            if (pData)
            {
                setCustomXData(pData);
            }
        }
    }
}

QColor PlotCurve::getLineColor() const
{
    return mLineColor;
}


void PlotCurve::resetLegendSize()
{
    // For now hardcoded but maybe in the future be possible to select, (default 8x8 is to small to see difference between dashed and solid lines)
    setLegendIconSize(QSize(40,12));
}


//! @brief Changes a curve to the previous available gneraetion of its data
void PlotCurve::setPreviousGeneration()
{
    // Loop until we find next lower generation, abort if gen<0
    int gen = getGeneration()-1;
    while ((gen >= 0) && (gen >= mpData->getLowestGeneration())  && !setGeneration(gen))
    {
        --gen;
    }
}


//! @brief Changes a curve to the next available generation of its data
void PlotCurve::setNextGeneration()
{
    // Loop until we find next higher generation, abort if we reach the highest
    int gen = getGeneration()+1;
    while ((gen <= mpData->getHighestGeneration()) && !setGeneration(gen))
    {
        ++gen;
    }
}


//! @brief Sets the line width of a plot curve
//! @param lineWidth Line width to give curve
void PlotCurve::setLineWidth(int lineWidth)
{
    mLineWidth = lineWidth;
    QPen tempPen = pen();
    // Add one pt extra width for active curves
    if (mIsActive)
    {
        tempPen.setWidth(lineWidth+1);
    }
    else
    {
        tempPen.setWidth(lineWidth);
    }
    setPen(tempPen);
}


void PlotCurve::setLineStyle(QString lineStyle)
{
    mLineStyle = lineStyle;
    setStyle(PlotCurve::Lines); //Assume we want lines
    QPen tempPen = pen();
    if(lineStyle == "Solid Line")
    {
        tempPen.setStyle(Qt::SolidLine);
    }
    else if(lineStyle == "Dash Line")
    {
        tempPen.setStyle(Qt::DashLine);
    }
    else if(lineStyle == "Dot Line")
    {
        tempPen.setStyle(Qt::DotLine);
    }
    else if(lineStyle == "Dash Dot Line")
    {
        tempPen.setStyle(Qt::DashDotLine);
    }
    else if(lineStyle == "Dash Dot Dot Line")
    {
        tempPen.setStyle(Qt::DashDotDotLine);
    }
    else
    {
        // Deactivate line completely
        setStyle(PlotCurve::NoCurve);
    }
    setPen(tempPen);
    resetLegendSize();
}

void PlotCurve::setLineSymbol(QString lineSymbol)
{
    mLineSymbol = lineSymbol;
    mpCurveSymbol = new QwtSymbol();
    if(lineSymbol == "Cross")
    {
        mpCurveSymbol->setStyle(QwtSymbol::Cross);
    }
    else if(lineSymbol == "XCross")
    {
        mpCurveSymbol->setStyle(QwtSymbol::XCross);
    }
    else if(lineSymbol == "Ellipse")
    {
        mpCurveSymbol->setStyle(QwtSymbol::Ellipse);
    }
    else if(lineSymbol == "Star 1")
    {
        mpCurveSymbol->setStyle(QwtSymbol::Star1);
    }
    else if(lineSymbol == "Star 2")
    {
        mpCurveSymbol->setStyle(QwtSymbol::Star2);
    }
    else if(lineSymbol == "Hexagon")
    {
        mpCurveSymbol->setStyle(QwtSymbol::Hexagon);
    }
    else if(lineSymbol == "Rectangle")
    {
        mpCurveSymbol->setStyle(QwtSymbol::Rect);
    }
    else if(lineSymbol == "Horizontal Line")
    {
        mpCurveSymbol->setStyle(QwtSymbol::HLine);
    }
    else if(lineSymbol == "Vertical Line")
    {
        mpCurveSymbol->setStyle(QwtSymbol::VLine);
    }
    else if(lineSymbol == "Diamond")
    {
        mpCurveSymbol->setStyle(QwtSymbol::Diamond);
    }
    else if(lineSymbol == "Triangle")
    {
        mpCurveSymbol->setStyle(QwtSymbol::Triangle);
    }
    else if(lineSymbol == "Up Triangle")
    {
        mpCurveSymbol->setStyle(QwtSymbol::UTriangle);
    }
    else if(lineSymbol == "Down Triangle")
    {
        mpCurveSymbol->setStyle(QwtSymbol::DTriangle);
    }
    else if(lineSymbol == "Right Triangle")
    {
        mpCurveSymbol->setStyle(QwtSymbol::RTriangle);
    }
    else if(lineSymbol == "Left Triangle")
    {
        mpCurveSymbol->setStyle(QwtSymbol::LTriangle);
    }
    else
    {
        mpCurveSymbol->setStyle(QwtSymbol::NoSymbol);
    }

    QPen tempPen = pen();
    tempPen.setStyle(Qt::SolidLine);
    mpCurveSymbol->setPen(tempPen);
    mpCurveSymbol->setSize(mCurveSymbolSize,mCurveSymbolSize);
    setSymbol(mpCurveSymbol);

    //! @todo Add a color or size picker for the markers
    resetLegendSize();
}

//! @brief Sets the color of a line
//! @brief color Color to give the line.
void PlotCurve::setLineColor(QColor color)
{
    QPen tempPen;
    mLineColor = color;

    // Set line color
    tempPen = pen();
    tempPen.setColor(color);
    setPen(tempPen);

    // Set symbol color, (but only if we have one, else an empty symbold will be created)
    if (mpCurveSymbol)
    {
        // Need to recreate the symbol so that legend will update
        setLineSymbol(mLineSymbol);
    }

    emit colorChanged(color);
}


//! @brief Sets the color of a line
//! @param colorName Svg name of the color
//! @see setLineColor(QColor color)
void PlotCurve::setLineColor(QString colorName)
{
    QColor color;
    if(colorName.isEmpty())
    {
        color = QColorDialog::getColor(pen().color(), mpParentPlotArea);
        if (!color.isValid()) { return; }
    }
    else
    {
        color = QColor(colorName);
    }
    setLineColor(color);
}


//! @brief Opens the scaling dialog for a plot curve
void PlotCurve::openScaleDialog()
{
    QDialog *pScaleDialog = new QDialog(mpParentPlotArea);
    pScaleDialog->setWindowTitle("Change plot-scale and plot-offsets");

    QLabel *pYPlotScale = new QLabel(pScaleDialog);
    QLabel *pYPlotScaleUnit = new QLabel(pScaleDialog);
    if (mpData)
    {
        pYPlotScale->setText(QString("%1").arg(mpData->getPlotScale()));
        pYPlotScaleUnit->setText(mpData->getPlotScaleDataUnit());
    }
    else
    {
        pYPlotScale->setText("0");
        pYPlotScale->setEnabled(false);
        pYPlotScaleUnit->setEnabled(false);
    }

    mpDataPlotOffsetLineEdit = new QLineEdit(pScaleDialog);
    mpDataPlotOffsetLineEdit->setValidator(new QDoubleValidator(mpDataPlotOffsetLineEdit));
    mpDataPlotOffsetLineEdit->setText(QString("%1").arg(mpData->getPlotOffset()));

    QLabel *pCurveUnitScale = new QLabel(pScaleDialog);
    QLabel *pCurveUnitScaleUnit = new QLabel(pScaleDialog);
    pCurveUnitScale->setText(QString("%1").arg(mCustomCurveDataUnitScale));
    pCurveUnitScaleUnit->setText(mCustomCurveDataUnit);

    mpLocalCurveScaleLineEdit = new QLineEdit(pScaleDialog);
    mpLocalCurveScaleLineEdit->setValidator(new QDoubleValidator(mpLocalCurveScaleLineEdit));
    mpLocalCurveScaleLineEdit->setText(QString("%1").arg(mLocalAdditionalCurveScale));

    mpLocalCurveOffsetLineEdit = new QLineEdit(pScaleDialog);
    mpLocalCurveOffsetLineEdit->setValidator(new QDoubleValidator(mpLocalCurveOffsetLineEdit));
    mpLocalCurveOffsetLineEdit->setText(QString("%1").arg(mLocalAdditionalCurveOffset));


    QPushButton *pDoneButton = new QPushButton("Done", pScaleDialog);
    QDialogButtonBox *pButtonBox = new QDialogButtonBox(Qt::Horizontal);
    pButtonBox->addButton(pDoneButton, QDialogButtonBox::ActionRole);

    QGridLayout *pDialogLayout = new QGridLayout(pScaleDialog);
    QLabel *pName = new QLabel(this->getCurveName() + QString(",     Generation: %1").arg(this->getGeneration()+1), pScaleDialog);
    QFont font = pName->font();
    font.setBold(true);
    pName->setFont(font);
    int r=0;
    pDialogLayout->addWidget(pName, r,0,1,2,Qt::AlignLeft);
    ++r;
    //Space
    pDialogLayout->setRowMinimumHeight(r,12);
    ++r;
    pDialogLayout->addWidget(new QLabel("Data plot scale and offset that affects all plot curves based on this data variable:", pScaleDialog),r,0,1,2,Qt::AlignLeft);
    ++r;
    pDialogLayout->addWidget(new QLabel("Data plot scale: ", pScaleDialog),         r,0);
    pDialogLayout->addWidget(pYPlotScale,                                           r,1);
    pDialogLayout->addWidget(pYPlotScaleUnit,                                       r,2);
    ++r;
    pDialogLayout->addWidget(new QLabel("Data plot offset: ", pScaleDialog),        r,0);
    pDialogLayout->addWidget(mpDataPlotOffsetLineEdit,                              r,1);
    ++r;
    //Space
    pDialogLayout->setRowMinimumHeight(r,12);
    ++r;
    pDialogLayout->addWidget(new QLabel("Plot curve scale and offset that affects only this plot curve (all generations):", pScaleDialog),r,0,1,2,Qt::AlignLeft);
    ++r;
    pDialogLayout->addWidget(new QLabel("Plot curve unit scale: ", pScaleDialog),   r,0);
    pDialogLayout->addWidget(pCurveUnitScale,                                       r,1);
    pDialogLayout->addWidget(pCurveUnitScaleUnit,                                   r,2);
    ++r;
    pDialogLayout->addWidget(new QLabel("Plot curve scale: ", pScaleDialog),        r,0);
    pDialogLayout->addWidget(mpLocalCurveScaleLineEdit,                             r,1);
    ++r;
    pDialogLayout->addWidget(new QLabel("Plot curve offset: ", pScaleDialog),       r,0);
    pDialogLayout->addWidget(mpLocalCurveOffsetLineEdit,                            r,1);
    ++r;

    pDialogLayout->addWidget(pButtonBox,r,0,1,2);
    pScaleDialog->setLayout(pDialogLayout);

    connect(pDoneButton,                SIGNAL(clicked()),pScaleDialog, SLOT(close()));
    connect(mpLocalCurveScaleLineEdit,  SIGNAL(textChanged(QString)),   SLOT(updateLocalPlotScaleAndOffsetFromDialog()));
    connect(mpLocalCurveOffsetLineEdit, SIGNAL(textChanged(QString)),   SLOT(updateLocalPlotScaleAndOffsetFromDialog()));
    connect(mpDataPlotOffsetLineEdit,   SIGNAL(textChanged(QString)),   SLOT(updateDataPlotOffsetFromDialog()));

    pScaleDialog->exec();

    // Disconnect again to avoid triggering value update the next time the dialog is built
    disconnect(mpLocalCurveScaleLineEdit, 0, 0, 0);
    disconnect(mpLocalCurveOffsetLineEdit, 0, 0, 0);
    disconnect(mpDataPlotOffsetLineEdit, 0, 0, 0);

    pScaleDialog->deleteLater();
}


//! @brief Updates the scaling of a plot curve from values in scaling dialog
//! @todo FIXA  /Peter
void PlotCurve::updateTimePlotScaleFromDialog()
{
    double newScale = mpTimeScaleComboBox->currentText().split(" ")[0].toDouble();
    double oldScale = mpData->getSharedTimeOrFrequencyVector()->getPlotScale();

    setTimePlotScalingAndOffset(newScale, mpTimeOffsetSpinBox->value());

    // Update zoom rectangle to new scale if zoomed
    if(mpParentPlotArea->isZoomed())
    {
        QRectF oldZoomRect = mpParentPlotArea->mpQwtZoomerLeft->zoomRect();
        QRectF newZoomRect = QRectF(oldZoomRect.x()*newScale/oldScale, oldZoomRect.y(), oldZoomRect.width()*newScale/oldScale, oldZoomRect.height());

        mpParentPlotArea->resetZoom();

        mpParentPlotArea->mpQwtZoomerLeft->zoom(newZoomRect);
        mpParentPlotArea->replot();
    }

    mpParentPlotArea->mpQwtPlot->setAxisTitle(QwtPlot::xBottom, "Time ["+mpTimeScaleComboBox->currentText().split(" ")[1].remove("(").remove(")")+"] ");     //!< @todo Not so nice fix... /Robert
}

void PlotCurve::updateLocalPlotScaleAndOffsetFromDialog()
{
    setLocalCurvePlotScaleAndOffset(mpLocalCurveScaleLineEdit->text().toDouble(), mpLocalCurveOffsetLineEdit->text().toDouble());
}

void PlotCurve::updateDataPlotOffsetFromDialog()
{
    setDataPlotOffset(mpDataPlotOffsetLineEdit->text().toDouble());
}


//! @brief Updates a plot curve to the most recent available generation of its data
void PlotCurve::updateToNewGeneration()
{
    // Only change the generation if auto update is on
    if(mAutoUpdate)
    {
        setGeneration(-1);
    }
    // Update the plot info box regardless of auto update setting, to show number of available generations correctly
    emit curveInfoUpdated();
}

//! @brief Activates (highlights) the plot curve
void PlotCurve::markActive(bool value)
{
    if(value)
    {
        mIsActive = true;
        //! @todo setZ to show selected on top, changes the actual curve order and legent order which looks strange, need to solve that somehow
        //setZ(ActiveCurveZOrderType);
    }
    else
    {
        mIsActive = false;
        //setZ(CurveZOrderType);
    }

    setLineWidth(mLineWidth);
    emit markedActive(mIsActive);
}


//! @brief Redraws the curve
//! Updates a curve with regard to special X-axis, units and scaling.
//! @todo after updating from python, scale is not refreshed maybe this should be done in here
//! @todo add optional index if we only want to update particular value
void PlotCurve::updateCurve()
{
    // Handle complex variables in a special way
    if (mpData->getVariableType() == ComplexType)
    {
        ComplexVectorVariable *pComplexVar = qobject_cast<ComplexVectorVariable*>(mpData.data());
        if (pComplexVar)
        {
            setSamples(pComplexVar->getRealDataCopy(), pComplexVar->getImagDataCopy());
        }
    }
    else
    {
        QVector<double> tempX, tempY;
        // We copy here, it should be faster then peek (at least when data is cached on disc)
        //! @todo maybe be smart about doing this copy
        tempY = mpData->getDataVectorCopy();
        const double dataScale = mpData->getPlotScale();
        const double dataOffset = mpData->getPlotOffset();

        if(mpCustomXdata.isNull())
        {
            const double yScale = mLocalAdditionalCurveScale*mCustomCurveDataUnitScale*dataScale;
            const double yOffset = dataOffset + mLocalAdditionalCurveOffset;

            // No special X-data use time vector if it exist else we cant draw curve (yet, x-date might be set later)
            if (mpData->getSharedTimeOrFrequencyVector())
            {
                tempX = mpData->getSharedTimeOrFrequencyVector()->getDataVectorCopy();
                const double timeScale = mpData->getSharedTimeOrFrequencyVector()->getPlotScale();
                const double timeOffset = mpData->getSharedTimeOrFrequencyVector()->getPlotOffset();

                for(int i=0; i<tempX.size() && i<tempY.size(); ++i)
                {
                    tempX[i] = tempX[i]*timeScale + timeOffset;
                    tempY[i] = tempY[i]*yScale + yOffset;
                }
            }
            else
            {
                // No timevector or special x-vector, plot vs samples
                tempX.resize(tempY.size());
                for (int i=0; i< tempX.size(); ++i)
                {
                    tempX[i] = i;
                    tempY[i] = tempY[i]*yScale + yOffset;
                }
            }
        }
        else
        {
            const double yScale = mLocalAdditionalCurveScale*mCustomCurveDataUnitScale*dataScale;
            const double yOffset = dataOffset + mLocalAdditionalCurveOffset;

            // Use special X-data
            // We copy here, it should be faster then peek (at least when data is cached on disc)
            tempX = mpCustomXdata->getDataVectorCopy();
            const double xScale = mpCustomXdata->getPlotScale();
            const double xOffset = mpCustomXdata->getPlotOffset();
            for(int i=0; i<tempX.size() && i<tempY.size(); ++i)
            {
                tempX[i] = tempX[i]*xScale + xOffset;
                tempY[i] = tempY[i]*yScale + yOffset;
            }
        }

        setSamples(tempX, tempY);
    }

    emit curveDataUpdated();
}

void PlotCurve::updateCurveName()
{
    refreshCurveTitle();
    emit curveInfoUpdated();
}

void PlotCurve::deleteCustomData()
{
    if (mHaveCustomData)
    {
        mpData.clear();
        mHaveCustomData = false;
    }
}

void PlotCurve::connectDataSignals()
{
    //! @todo Might be better to connect to a signal from the variable container instead, then we only trigger if our data might have new information
    //! @todo what will happen if you import or set alias with same name as data then this will also trigger
    if (mpData->getLogDataHandler())
    {
        connect(mpData->getLogDataHandler(), SIGNAL(dataAdded()), this, SLOT(updateToNewGeneration()), Qt::UniqueConnection);
    }

    connect(mpData.data(), SIGNAL(dataChanged()), this, SLOT(updateCurve()), Qt::UniqueConnection);
    connect(mpData.data(), SIGNAL(nameChanged()), this, SLOT(updateCurveName()), Qt::UniqueConnection);
}

void PlotCurve::connectCustomXDataSignals()
{
    if (mpCustomXdata)
    {
        connect(mpCustomXdata.data(), SIGNAL(dataChanged()), this, SLOT(updateCurve()), Qt::UniqueConnection);
    }
}

void PlotCurve::disconnectDataSignals()
{
    //! @todo new gen signal
    if (mpData)
    {
        // Disconnect all data signals from mpData to this
        mpData.data()->disconnect(this);

        //! @todo Might be better to connect to a signal from the variable container instead, then we only trigger if our data might have new information
        //! @todo what will happen if you import or set alias with same name as data then this will also trigger
        if (mpData->getLogDataHandler())
        {
            mpData->getLogDataHandler()->disconnect(this);
        }
    }
}

void PlotCurve::disconnectCustomXDataSignals()
{
    if (mpCustomXdata)
    {
        // Disconnect all data signals from any custom x-data
        mpCustomXdata.data()->disconnect(this);
    }
}


//! @brief Sets auto update flag for a plot curve
//! If this is activated, plot will automatically change to latest plot generation after next simulation.
void PlotCurve::setAutoUpdate(bool value)
{
    mAutoUpdate = value;
}


void PlotCurve::openFrequencyAnalysisDialog()
{
    mpParentPlotArea->mpParentPlotTab->openFrequencyAnalysisDialog(this);
}



//! @brief Constructor for plot markers
//! @param pCurve Pointer to curve the marker belongs to
//! @param pPlotTab Plot tab the marker is located in
//! @param markerSymbol The symbol the marker shall use
PlotMarker::PlotMarker(PlotCurve *pCurve, PlotArea *pPlotTab)
    : QwtPlotMarker()
{
    mpCurve = pCurve;
    mpPlotArea = pPlotTab;
    mIsBeingMoved = false;
    mIsMovable = true;
    mMarkerSize = 12;
    mLabelAlignment = Qt::AlignTop;

    mpMarkerSymbol = new QwtSymbol();
    mpMarkerSymbol->setStyle(QwtSymbol::XCross);
    mpMarkerSymbol->setSize(mMarkerSize,mMarkerSize);
    this->setRenderHint(QwtPlotItem::RenderAntialiased);

    setSymbol(mpMarkerSymbol); //!< @todo is this symbol auto removed with PlotMarker ?
    this->setZ(CurveMarkerZOrderType);

    this->setColor(pCurve->pen().color());

    connect(mpCurve, SIGNAL(colorChanged(QColor)), this, SLOT(setColor(QColor)));
}


//! @brief Event filter for plot markers
//! This will interrupt events from plot canvas, to enable using mouse and key events for modifying markers.
//! @returns True if event was interrupted, false if its propagation shall continue
//! @param object Pointer to the object the event belongs to (in this case the plot canvas)
//! @param event Event to be interrupted
bool PlotMarker::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object);

    if (event->type() == QEvent::ContextMenu)
    {
        QPointF midPoint;
        midPoint.setX(this->plot()->transform(QwtPlot::xBottom, value().x()));
        midPoint.setY(this->plot()->transform(QwtPlot::yLeft, value().y()));
        if((this->plot()->canvas()->mapToGlobal(midPoint.toPoint()) - QCursor::pos()).manhattanLength() < 35)
        {
            QMenu *pContextMenu = new QMenu();

            // Line style selection menu
            QMenu *pLineStyleMenu = new QMenu("Line Style");
            pContextMenu->addMenu(pLineStyleMenu);
            QAction *pNoLinesAction = new QAction("No Lines", pContextMenu);
            QAction *pVerticalLineAction = new QAction("Vertical Line", pContextMenu);
            QAction *pHorizontalLineAction = new QAction("Horizontal Line", pContextMenu);
            QAction *pCrossAction = new QAction("Cross", pContextMenu);
            pLineStyleMenu->addAction(pNoLinesAction);
            pLineStyleMenu->addAction(pVerticalLineAction);
            pLineStyleMenu->addAction(pHorizontalLineAction);
            pLineStyleMenu->addAction(pCrossAction);

            // Label alignment selection menu
            QMenu *pAlignmentMenu = new QMenu("Label Alignment");
            pContextMenu->addMenu(pAlignmentMenu);
            QList<AlignmentSelectionStruct> alignments;
            alignments.append(AlignmentSelectionStruct(Qt::AlignTop|Qt::AlignLeft, "Top Left"));
            alignments.append(AlignmentSelectionStruct(Qt::AlignTop, "Top"));
            alignments.append(AlignmentSelectionStruct(Qt::AlignTop|Qt::AlignRight, "Top Right"));
            alignments.append(AlignmentSelectionStruct(Qt::AlignRight, "Right"));
            alignments.append(AlignmentSelectionStruct(Qt::AlignBottom|Qt::AlignRight, "Bottom Right"));
            alignments.append(AlignmentSelectionStruct(Qt::AlignBottom, "Bottom"));
            alignments.append(AlignmentSelectionStruct(Qt::AlignBottom|Qt::AlignLeft, "Bottom Left"));
            alignments.append(AlignmentSelectionStruct(Qt::AlignLeft, "Left"));
            QAction *pAction;
            QMap<QAction*, AlignmentSelectionStruct*> alignSelectionMap;
            for (int i=0; i<alignments.size(); ++i)
            {
                pAction = new QAction(alignments[i].mLabel,pContextMenu);
                pAlignmentMenu->addAction(pAction);
                alignSelectionMap.insert(pAction, &alignments[i]);
            }

            // Delete marker action
            QAction *pDeleteAction = pContextMenu->addAction("Remove Marker");


            // Execute selected action
            pAction = pContextMenu->exec(QCursor::pos());
            if (pAction == pDeleteAction)
            {
                mpPlotArea->removePlotMarker(this);
            }
            else if(pAction == pNoLinesAction)
            {
                this->setLineStyle(NoLine);
            }
            else if(pAction == pVerticalLineAction)
            {
                this->setLineStyle(VLine);
            }
            else if(pAction == pHorizontalLineAction)
            {
                this->setLineStyle(HLine);
            }
            else if(pAction == pCrossAction)
            {
                this->setLineStyle(Cross);
            }
            else if(alignSelectionMap.contains(pAction))
            {
                mLabelAlignment = alignSelectionMap.value(pAction)->mAlignment;
                this->setLabelAlignment(mLabelAlignment);
            }

            pContextMenu->deleteLater();
            return true;
        }
    }

    // Mouse press events, used to initiate moving of a marker if mouse cursor is close enough
    else if (event->type() == QEvent::MouseButtonPress)
    {
        if(!mIsMovable)
            return false;

        if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
        {
            QCursor cursor;
            QPointF midPoint;
            midPoint.setX(this->plot()->transform(QwtPlot::xBottom, value().x()));
            midPoint.setY(this->plot()->transform(QwtPlot::yLeft, value().y()));

            if(!mpPlotArea->mpQwtZoomerLeft->isEnabled() && !mpPlotArea->mpQwtPanner->isEnabled())
            {
                if((this->plot()->canvas()->mapToGlobal(midPoint.toPoint()) - cursor.pos()).manhattanLength() < 35)
                {
                    mIsBeingMoved = true;
                    return true;
                }
            }
        }
    }

    // Mouse move (hover) events, used to change marker color or move marker if cursor is close enough.
    else if (event->type() == QEvent::MouseMove)
    {
        if(!mIsMovable)
            return false;

        bool retval = false;
        QCursor cursor;
        QPointF midPoint;
        midPoint.setX(this->plot()->transform(QwtPlot::xBottom, value().x()));
        midPoint.setY(this->plot()->transform(QwtPlot::yLeft, value().y()));
        if((this->plot()->canvas()->mapToGlobal(midPoint.toPoint()) - cursor.pos()).manhattanLength() < 35)
        {
            QColor tempColor = mpCurve->pen().color();
            tempColor.setAlpha(150);
            mpMarkerSymbol->setPen(tempColor.lighter(165), 3);
            this->plot()->replot();
            this->plot()->updateGeometry();
            retval=true;
        }
        else
        {
            if(!mIsBeingMoved)
            {
                QColor tempColor = mpCurve->pen().color();
                tempColor.setAlpha(150);
                mpMarkerSymbol->setPen(tempColor, 3);
                this->plot()->replot();
                this->plot()->updateGeometry();
            }
        }

        if(mIsBeingMoved)
        {
            double x = mpCurve->sample(mpCurve->closestPoint(this->plot()->canvas()->mapFromGlobal(cursor.pos()))).x();
            double y = mpCurve->sample(mpCurve->closestPoint(this->plot()->canvas()->mapFromGlobal(cursor.pos()))).y();
            setXValue(x);
            setYValue(this->plot()->invTransform(QwtPlot::yLeft, this->plot()->transform(mpCurve->yAxis(), y)));

            refreshLabel(x, y);
        }
        return retval;
    }

    // Mouse release event, will stop moving marker
    else if (event->type() == QEvent::MouseButtonRelease && mIsBeingMoved == true)
    {
        mIsBeingMoved = false;
        return false;
    }

    // Keypress event, will delete marker if delete key is pressed
    else if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() == Qt::Key_Delete)
        {
            QCursor cursor;
            QPointF midPoint;
            midPoint.setX(this->plot()->transform(QwtPlot::xBottom, value().x()));
            midPoint.setY(this->plot()->transform(mpCurve->yAxis(), value().y()));
            if((this->plot()->canvas()->mapToGlobal(midPoint.toPoint()) - cursor.pos()).manhattanLength() < 35)
            {
                mpPlotArea->removePlotMarker(this);
                return true;
            }
        }
        return false;
    }
    return false;
}


void PlotMarker::setMovable(bool movable)
{
    mIsMovable = movable;
}

void PlotMarker::refreshLabel(const double x, const double y)
{
    refreshLabel(QString("( %1,  %2 )").arg(x).arg(y));
}

void PlotMarker::refreshLabel(const QString &label)
{
    QwtText qwtlabel(label);
    qwtlabel.setColor(Qt::black);
    qwtlabel.setBackgroundBrush(QColor(255,255,255,240));
    qwtlabel.setFont(QFont("Calibri", 14, QFont::Normal));
    setLabel(qwtlabel);
    setLabelAlignment(mLabelAlignment);
}


void PlotMarker::setColor(QColor color)
{
    color.setAlpha(150);    //Markers should be semi-transparent
    mpMarkerSymbol->setPen(color,3);
    this->setLinePen(color,2, Qt::DotLine);
}



//! @brief Returns a pointer to the curve a plot marker belongs to
PlotCurve *PlotMarker::getCurve()
{
    return mpCurve;
}


QList<QwtLegendData> PlotCurve::legendData() const
{
    QList<QwtLegendData> list = QwtPlotCurve::legendData();
    for (int i=0; i<list.size(); ++i)
    {
        list[i].setValue( AxisIdRole, this->yAxis() );
    }
    return list;
}

PlotLegend::PlotLegend(QwtPlot::Axis axisId) :
    QwtPlotLegendItem()
{
    setMaxColumns(1);
    setRenderHint(QwtPlotItem::RenderAntialiased);
    setBackgroundMode(LegendBackground);
    setBackgroundBrush(QColor(Qt::white));
    setBorderRadius(8);
    setMargin(4);
    setSpacing(2);
    setItemMargin(0);
    QFont font = this->font();
    font.setPointSize(11);
    setFont(font);

    mAxis = axisId;
}

void PlotLegend::updateLegend(const QwtPlotItem *plotItem, const QList<QwtLegendData> &data)
{
    // Use only those curve pointers that should belong to this particular legend
    QList<QwtLegendData> myData;
    for (int i=0; i<data.size(); ++i)
    {
        if (data[i].value(AxisIdRole) == mAxis)
        {
            myData.push_back(data[i]);
        }
    }

    QwtPlotLegendItem::updateLegend( plotItem, myData );
}

