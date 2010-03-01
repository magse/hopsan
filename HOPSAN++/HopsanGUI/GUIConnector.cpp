//$Id$

#include "GUIConnector.h"
#include <QDebug>

#include <QCursor>
#include <QBrush>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QCursor>
#include <QBrush>
#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include "GUIPort.h"
#include "assert.h"
#include <vector>
#include "GUIConnectorLine.h"


GUIConnector::GUIConnector(qreal x1, qreal y1, qreal x2, qreal y2, QPen passivePen, QPen activePen, QPen hoverPen, QGraphicsView *parentView, QGraphicsItem *parent)
        : QGraphicsWidget(parent)
{
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
    this->setPos(x1, y1);
    this->startPos.setX(x1);
    this->startPos.setY(y1);
    this->endPos.setX(x2);
    this->endPos.setY(y2);
    this->mpParentView = parentView;
    this->mPassivePen = passivePen;
    this->mActivePen = activePen;
    this->mHoverPen = hoverPen;
    this->mIsActive = false;
    this->mEndPortConnected = false;
    mpTempLine = new GUIConnectorLine(this->mapFromScene(startPos).x(), this->mapFromScene(startPos).y(),
                                      this->mapFromScene(startPos).x(), this->mapFromScene(startPos).y(),
                                      mPassivePen, mActivePen, mHoverPen, 0, this);
    mLines.push_back(mpTempLine);
    connect(mLines[mLines.size()-1],SIGNAL(lineClicked()),this,SLOT(doSelect()));
    connect(mLines[mLines.size()-1],SIGNAL(lineHoverEnter()),this,SLOT(setHovered()));
    connect(mLines[mLines.size()-1],SIGNAL(lineHoverLeave()),this,SLOT(setUnHovered()));
    this->setActive();
    connect(this->mpParentView,SIGNAL(keyPressDelete()),this,SLOT(deleteMeIfMeIsActive()));
}

GUIConnector::~GUIConnector()
{
}

void GUIConnector::SetEndPos(qreal x2, qreal y2)
{
    this->endPos.setX(x2);
    this->endPos.setY(y2);
}

void GUIConnector::setStartPort(GUIPort *port)
{
    this->mpStartPort = port;
    connect(this->mpStartPort->getComponent(),SIGNAL(componentMoved()),this,SLOT(updatePos()));
    connect(this->mpStartPort->getComponent(),SIGNAL(componentDeleted()),this,SLOT(deleteMe()));
}

void GUIConnector::setEndPort(GUIPort *port)
{
    this->mpEndPort = port;
    this->mEndPortConnected = true;
    connect(this->mpEndPort->getComponent(),SIGNAL(componentMoved()),this,SLOT(updatePos()));

    qDebug() << this->boundingRect().x() << " " << this->boundingRect().y() << " ";
    connect(this->mpEndPort->getComponent(),SIGNAL(componentDeleted()),this,SLOT(deleteMe()));
    for(std::size_t i=1; i!=mLines.size()-1; ++i)
    {
        mLines[i]->setFlags(QGraphicsItem::ItemSendsGeometryChanges | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemUsesExtendedStyleOption);
    }
}

GUIPort *GUIConnector::getStartPort()
{
    return this->mpStartPort;
}

GUIPort *GUIConnector::getEndPort()
{
    return this->mpEndPort;
}

void GUIConnector::updatePos()
{
    QPointF startPort = this->getStartPort()->mapToScene(this->getStartPort()->boundingRect().center());
    QPointF endPort = this->getEndPort()->mapToScene(this->getEndPort()->boundingRect().center());
    this->drawLine(startPort, endPort);
}

void GUIConnector::doSelect()
{
    if(this->mEndPortConnected)     //Non-finished lines shall not be selectable
    {
        this->setSelected(true);
        qDebug() << "doSelect()";
    }
}

void GUIConnector::setActive()
{
    if(this->mEndPortConnected)
    {
        mIsActive = true;
        for (std::size_t i=0; i!=mLines.size(); ++i )
        {
            mLines[i]->setActive();
        }
        qDebug() << "setActive()";
    }
}

void GUIConnector::setPassive()
{
    if(this->mEndPortConnected)
    {
        mIsActive = false;
        for (std::size_t i=0; i!=mLines.size(); ++i )
        {
            mLines[i]->setPassive();
        }
    }
}

void GUIConnector::setUnHovered()
{
    if(this->mEndPortConnected && !this->mIsActive)
    {
        for (std::size_t i=0; i!=mLines.size(); ++i )
        {
            mLines[i]->setPassive();
        }
    }
}

void GUIConnector::setHovered()
{
    if(this->mEndPortConnected && !this->mIsActive)
    {
        for (std::size_t i=0; i!=mLines.size(); ++i )
        {
            mLines[i]->setHovered();
        }
    }
}

void GUIConnector::drawLine(QPointF startPos, QPointF endPos)
{
    startPos = this->mapFromScene(startPos);
    endPos = this->mapFromScene(endPos);


    //First two lines out of the component:
    if (getNumberOfLines()<3)
    {
        getLastLine()->setLine(startPos.x(),
                               startPos.y(),
                               endPos.x(),
                               startPos.y());
        getLastLine()->setGeometry(GUIConnectorLine::HORIZONTAL);
        getThisLine()->setGeometry(GUIConnectorLine::VERTICAL);
    }
    //If last line was vertical:
    else if (getLastLine()->getGeometry()== GUIConnectorLine::VERTICAL and getThisLine()->getGeometry()!=GUIConnectorLine::DIAGONAL)
    {
        getLastLine()->setLine(getOldLine()->line().x2(),
                               getOldLine()->line().y2(),
                               getOldLine()->line().x2(),
                               endPos.y());
        getThisLine()->setGeometry(GUIConnectorLine::HORIZONTAL);
    }
    //If last line was horizontal:
    else if (getLastLine()->getGeometry()==GUIConnectorLine::HORIZONTAL and getThisLine()->getGeometry()!=GUIConnectorLine::DIAGONAL)
    {
        getLastLine()->setLine(getOldLine()->line().x2(),
                               getOldLine()->line().y2(),
                               endPos.x(),
                               getOldLine()->line().y2());
        getThisLine()->setGeometry(GUIConnectorLine::VERTICAL);
    }
    //If the line is diagonal:
//    if (getThisLine()->getGeometry()==GUIConnectorLine::DIAGONAL)
//    {
//        getThisLine()->setLine(getLastLine()->line().x2(),
//                               getLastLine()->line().y2(),
//                               endPos.x(),
//                               endPos.y());
//    }
    //This Line:
    getThisLine()->setLine(getLastLine()->line().x2(),
                           getLastLine()->line().y2(),
                           endPos.x(),
                           endPos.y());
}

void GUIConnector::setPen(QPen pen)
{
    for (std::size_t i=0; i!=mLines.size(); ++i )
    {
        mLines[i]->setPen(pen);
    }
}

void GUIConnector::addLine()
{
    mpTempLine = new GUIConnectorLine(mLines[mLines.size()-1]->line().p2().x(), mLines[mLines.size()-1]->line().p2().y(),
                                      mLines[mLines.size()-1]->line().p2().x(), mLines[mLines.size()-1]->line().p2().y(),
                                      mPassivePen, mActivePen, mHoverPen, mLines.size(), this);
    mpTempLine->setActive();
    mLines.push_back(mpTempLine);
    mLines[mLines.size()-2]->setPassive();
    connect(mLines[mLines.size()-1],SIGNAL(lineClicked()),this,SLOT(doSelect()));
    connect(mLines[mLines.size()-1],SIGNAL(lineMoved(int)),this, SLOT(updateLine(int)));
    connect(mLines[mLines.size()-1],SIGNAL(lineHoverEnter()),this,SLOT(setHovered()));
    connect(mLines[mLines.size()-1],SIGNAL(lineHoverLeave()),this,SLOT(setUnHovered()));
}

void GUIConnector::removeLine(QPointF cursorPos)
{
    if (getNumberOfLines() > 2)
    {
        this->scene()->removeItem(mLines.back());
        mLines.pop_back();
        this->drawLine(this->mapToScene(mLines[0]->line().p1()), cursorPos);
    }
    else
    {

        this->scene()->removeItem(this);
        delete(this);
    }
}

int GUIConnector::getNumberOfLines()
{
    return mLines.size();
}

void GUIConnector::deleteMe()
{
    mLines.clear();
    this->scene()->removeItem(this);
    delete(this);
}


void GUIConnector::deleteMeIfMeIsActive()
{
    if(this->mIsActive && mLines.size() > 0)
    {
        mLines.clear();
        this->scene()->removeItem(this);
        delete(this);
    }
}

void GUIConnector::updateLine(int lineNumber)
{
    qDebug() << "Updating line: x = " << (mLines[lineNumber]->scenePos());
    if (this->mEndPortConnected && lineNumber != 0 && lineNumber != mLines.size())
    {
        if(mLines[lineNumber]->getGeometry()==GUIConnectorLine::HORIZONTAL)
        {
            mLines[lineNumber]->setLine(mLines[lineNumber]->mapFromItem(mLines[lineNumber-1], mLines[lineNumber-1]->line().p2()).x(),
                                        mLines[lineNumber]->line().y1(),
                                        mLines[lineNumber]->mapFromItem(mLines[lineNumber+1], mLines[lineNumber+1]->line().p1()).x(),
                                        mLines[lineNumber]->line().y2());
            mLines[lineNumber-1]->setLine(mLines[lineNumber-1]->line().x1(),
                                          mLines[lineNumber-1]->line().y1(),
                                          mLines[lineNumber-1]->line().x2(),
                                          mLines[lineNumber-1]->mapFromItem(mLines[lineNumber], mLines[lineNumber]->line().p1()).y());
            mLines[lineNumber+1]->setLine(mLines[lineNumber+1]->line().x1(),
                                          mLines[lineNumber+1]->mapFromItem(mLines[lineNumber], mLines[lineNumber]->line().p2()).y(),
                                          mLines[lineNumber+1]->line().x2(),
                                          mLines[lineNumber+1]->line().y2());
        }
        else if(mLines[lineNumber]->getGeometry()==GUIConnectorLine::VERTICAL)
        {
            mLines[lineNumber]->setLine(mLines[lineNumber]->line().x1(),
                                        mLines[lineNumber]->mapFromItem(mLines[lineNumber-1], mLines[lineNumber-1]->line().p2()).y(),
                                        mLines[lineNumber]->line().x2(),
                                        mLines[lineNumber]->mapFromItem(mLines[lineNumber+1], mLines[lineNumber+1]->line().p1()).y());
            mLines[lineNumber-1]->setLine(mLines[lineNumber-1]->line().x1(),
                                          mLines[lineNumber-1]->line().y1(),
                                          mLines[lineNumber-1]->mapFromItem(mLines[lineNumber], mLines[lineNumber]->line().p1()).x(),
                                          mLines[lineNumber-1]->line().y2());
            mLines[lineNumber+1]->setLine(mLines[lineNumber+1]->mapFromItem(mLines[lineNumber], mLines[lineNumber]->line().p2()).x(),
                                          mLines[lineNumber+1]->line().y1(),
                                          mLines[lineNumber+1]->line().x2(),
                                          mLines[lineNumber+1]->line().y2());
        }
    }
}


GUIConnectorLine *GUIConnector::getOldLine()
{
    return mLines[mLines.size()-3];
}

GUIConnectorLine *GUIConnector::getLastLine()
{
    return mLines[mLines.size()-2];
}


GUIConnectorLine *GUIConnector::getThisLine()
{
    return mLines[mLines.size()-1];
}


QVariant GUIConnector::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemSelectedChange)
    {
        qDebug() << "Line selection status = " << this->isSelected();
        if(this->isSelected())
        {
            this->setPassive();
        }
        else
        {
            this->setActive();
        }
    }
    return value;
}

