#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include "graphicsitem.h"
#include "mainwindow.h"

int Wire::autoWireNo=0;
void Wire::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	QStyleOptionGraphicsItem op;
	op.initFrom(widget);
    if (option->state & QStyle::State_Selected)
	{
		op.state = QStyle::State_None;//去掉默认虚线框
		painter->setPen(QPen(Qt::red,0,Qt::DashLine));//绘制虚线
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(boundingRect().adjusted(WIRE_WIDTH_TO_SELECT/2.0,WIRE_WIDTH_TO_SELECT/2.0,-WIRE_WIDTH_TO_SELECT/2.0,-WIRE_WIDTH_TO_SELECT/2.0));
	}
	QGraphicsLineItem::paint(painter,&op,widget);
}

QPainterPath Wire::shape() const
{
    QPainterPath path=QGraphicsLineItem::shape();
    QPainterPathStroker stroker;
    stroker.setWidth(WIRE_WIDTH_TO_SELECT);
    path=stroker.createStroke(path);
    return path;
}

void Wire::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	event->setScenePos(QPointF(snapToGrid(event->scenePos().x()),snapToGrid(event->scenePos().y())));
	event->setButtonDownScenePos(Qt::LeftButton,QPointF(snapToGrid(event->buttonDownScenePos(Qt::LeftButton).x()),snapToGrid(event->buttonDownScenePos(Qt::LeftButton).y())));
	QGraphicsLineItem::mouseMoveEvent(event);
	setPos(snapToGrid(pos().x()),snapToGrid(pos().y()));
	QList<QGraphicsItem *> tmpItem=scene()->items();
	QList<QGraphicsItem *>::iterator it;
	for(it=tmpItem.begin();it!=tmpItem.end();it++)//移除所有旧有交点标识
		if((*it)->type()==QGraphicsEllipseItem::Type)
		{
			scene()->removeItem((*it));
			delete *it;
		}
	tmpItem.clear();
	tmpItem=scene()->collidingItems(this);//重新标记交点
	for(it=tmpItem.begin();it!=tmpItem.end();it++)
		if((*it)->type()==Wire::Type)
			((GraphicsScene *)scene())->addCollidingSign(*it,this);
	scene()->update();
}

PartView::PartView(Element *_part = NULL) : part(_part)
{
	setAcceptDrops(true);
	setFlags(ItemIsSelectable|ItemIsMovable|ItemIsFocusable);
	setHandlesChildEvents(false);
}

void PartView::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	event->setScenePos(QPointF(snapToGrid(event->scenePos().x()),snapToGrid(event->scenePos().y())));
	event->setButtonDownScenePos(Qt::LeftButton,QPointF(snapToGrid(event->buttonDownScenePos(Qt::LeftButton).x()),snapToGrid(event->buttonDownScenePos(Qt::LeftButton).y())));
	QGraphicsItemGroup::mouseMoveEvent(event);
	setPos(snapToGrid(pos().x()),snapToGrid(pos().y()));
	QList<QGraphicsItem *> tmpItem=scene()->items();
	QList<QGraphicsItem *>::iterator it;
	for(it=tmpItem.begin();it!=tmpItem.end();it++)//移除所有旧有交点标识
		if((*it)->type()==QGraphicsEllipseItem::Type)
		{
			scene()->removeItem((*it));
			delete *it;
		}
	QList<Wire *>::iterator wit;
	tmpItem.clear();
	for(wit=partPin.begin();wit!=partPin.end();wit++)
		tmpItem+=scene()->collidingItems(*wit);//重新标记交点
	for(it=tmpItem.begin();it!=tmpItem.end();it++)
		if((*it)->type()==Wire::Type)
			for(wit=partPin.begin();wit!=partPin.end();wit++)
				if(*wit!=*it)
					((GraphicsScene *)scene())->addCollidingSign(*it,*wit);
	scene()->update();
}

void PartView::initPin()
{//初始化元件管脚
	if(part->pinNum==1)
	{
		Wire *pinView0=new Wire;
		addToGroup(pinView0);
        pinView0->setLine(PARTVIEW_LENGTH/2.0+DELTA,0.0,PARTVIEW_LENGTH/2.0+DELTA,PINVIEW_LENGTH);
		partPin.push_back(pinView0);
	}
	else if(part->pinNum==2)
	{
		Wire *pinView0=new Wire;
		Wire *pinView1=new Wire;
		addToGroup(pinView0);
		addToGroup(pinView1);
        pinView0->setLine(0.0,PARTVIEW_LENGTH/2.0+DELTA,PINVIEW_LENGTH,PARTVIEW_LENGTH/2.0+DELTA);
        pinView1->setLine(PARTVIEW_LENGTH-PINVIEW_LENGTH,PARTVIEW_LENGTH/2.0+DELTA,PARTVIEW_LENGTH,PARTVIEW_LENGTH/2.0+DELTA);
		partPin.push_back(pinView0);
		partPin.push_back(pinView1);
	}
	else if(part->pinNum==3)
	{
		Wire *pinView0=new Wire;
		Wire *pinView1=new Wire;
		Wire *pinView2=new Wire;
		addToGroup(pinView0);
		addToGroup(pinView1);
		addToGroup(pinView2);
        pinView0->setLine(PARTVIEW_LENGTH/2.0+DELTA,0.0,PARTVIEW_LENGTH/2.0+DELTA,PINVIEW_LENGTH);
        pinView1->setLine(0.0,PARTVIEW_LENGTH/2.0+DELTA,PINVIEW_LENGTH,PARTVIEW_LENGTH/2.0+DELTA);
        pinView2->setLine(PARTVIEW_LENGTH/2.0+DELTA,PARTVIEW_LENGTH-PINVIEW_LENGTH,PARTVIEW_LENGTH/2.0+DELTA,PARTVIEW_LENGTH);
		partPin.push_back(pinView0);
		partPin.push_back(pinView1);
		partPin.push_back(pinView2);
	}
    else if(part->pinNum==4)
    {
        Wire *pinView0=new Wire;
        Wire *pinView1=new Wire;
        Wire *pinView2=new Wire;
        Wire *pinView3=new Wire;
        addToGroup(pinView0);
        addToGroup(pinView1);
        addToGroup(pinView2);
        addToGroup(pinView3);
        if(part->type=="F")
        {
            pinView2->setLine(0.0,PARTVIEW_LENGTH*1.0/4.0+DELTA,PINVIEW_LENGTH,PARTVIEW_LENGTH*1.0/4.0+DELTA);
            pinView3->setLine(0.0,PARTVIEW_LENGTH*1.0/2.0+DELTA,PINVIEW_LENGTH,PARTVIEW_LENGTH*1.0/2.0+DELTA);
            pinView0->setLine(PARTVIEW_LENGTH-PINVIEW_LENGTH,PARTVIEW_LENGTH*1.0/2.0+DELTA,PARTVIEW_LENGTH,PARTVIEW_LENGTH*1.0/2.0+DELTA);
            pinView1->setLine(PARTVIEW_LENGTH-PINVIEW_LENGTH,PARTVIEW_LENGTH*1.0/4.0+DELTA,PARTVIEW_LENGTH,PARTVIEW_LENGTH*1.0/4.0+DELTA);
        }
        else if(part->type=="H")
        {
            pinView1->setLine(0.0,PARTVIEW_LENGTH*1.0/4.0+DELTA,PINVIEW_LENGTH,PARTVIEW_LENGTH*1.0/4.0+DELTA);
            pinView2->setLine(0.0,PARTVIEW_LENGTH*1.0/2.0+DELTA,PINVIEW_LENGTH,PARTVIEW_LENGTH*1.0/2.0+DELTA);
            pinView3->setLine(PARTVIEW_LENGTH-PINVIEW_LENGTH,PARTVIEW_LENGTH*1.0/2.0+DELTA,PARTVIEW_LENGTH,PARTVIEW_LENGTH*1.0/2.0+DELTA);
            pinView0->setLine(PARTVIEW_LENGTH-PINVIEW_LENGTH,PARTVIEW_LENGTH*1.0/4.0+DELTA,PARTVIEW_LENGTH,PARTVIEW_LENGTH*1.0/4.0+DELTA);
        }
        else
        {
            pinView0->setLine(0.0,PARTVIEW_LENGTH*1.0/4.0+DELTA,PINVIEW_LENGTH,PARTVIEW_LENGTH*1.0/4.0+DELTA);
            pinView1->setLine(0.0,PARTVIEW_LENGTH*1.0/2.0+DELTA,PINVIEW_LENGTH,PARTVIEW_LENGTH*1.0/2.0+DELTA);
            pinView2->setLine(PARTVIEW_LENGTH-PINVIEW_LENGTH,PARTVIEW_LENGTH*1.0/2.0+DELTA,PARTVIEW_LENGTH,PARTVIEW_LENGTH*1.0/2.0+DELTA);
            pinView3->setLine(PARTVIEW_LENGTH-PINVIEW_LENGTH,PARTVIEW_LENGTH*1.0/4.0+DELTA,PARTVIEW_LENGTH,PARTVIEW_LENGTH*1.0/4.0+DELTA);
        }
        partPin.push_back(pinView0);
        partPin.push_back(pinView1);
        partPin.push_back(pinView2);
        partPin.push_back(pinView3);
    }
	for(QList<Wire *>::iterator wit=partPin.begin();wit!=partPin.end();wit++)
	{
		(*wit)->setAcceptDrops(false);
		(*wit)->setFlag(QGraphicsItem::ItemIsSelectable,false);
		(*wit)->setFlag(QGraphicsItem::ItemIsMovable,false);
		(*wit)->setFlag(QGraphicsItem::ItemIsFocusable,false);
	}
}

void PartView::initViewText()
{//初始化元件标注
	//显示元件参数
	int i=0;
    if(part->type=="C"||part->type=="L")
    {
        partText.push_back(new PartViewText(QString::fromStdString(ftoe(part->showParameter(0))),this,0));
        partText.last()->setPos(PARTVIEW_LENGTH/2.0-10.0,PARTVIEW_LENGTH/2.0+20.0);
        partText.last()->setFont(QFont("Arial",10));
        i++;
    }
    else
    {
        if(part->type!="GND"&&part->type!="D"&&part->type!="NPN"&&part->type!="PNP"&&part->type!="NMOSFET"&&part->type!="PMOSFET")
            for(i=0;i<part->parameterNum;i++)
            {
                partText.push_back(new PartViewText(QString::fromStdString(ftoe(part->showParameter(i))),this,i));
                partText.last()->setPos(PARTVIEW_LENGTH/2.0-10.0,PARTVIEW_LENGTH/2.0+20.0+i*TEXT_HEIGHT);
                partText.last()->setFont(QFont("Arial",10));
            }
    }
	//显示元件名
    partText.push_back(new PartViewText(QString(part->name.c_str()),this,i));
	switch(part->pinNum)
	{
	case 1:partText.last()->setPos(PARTVIEW_LENGTH-50.0,PARTVIEW_LENGTH-TEXT_HEIGHT);break;
	case 2:partText.last()->setPos(PARTVIEW_LENGTH/2.0-10.0,-10.0);break;
    case 3:break;
    case 4:partText.last()->setPos(PARTVIEW_LENGTH/2.0-10.0,-20.0);break;
	}
	partText.last()->setFont(QFont("Arial",10));
}

void PartView::rotatePart()
{//逆时针旋转当前元件90°
    setTransformOriginPoint(PARTVIEW_LENGTH/2.0+DELTA,PARTVIEW_LENGTH/2.0+DELTA);
	setRotation(rotation()-90);
	if(partText.size()>1)
	{
		QPointF parameterTransformOriginPoint=(partText.first()->sceneBoundingRect().center()+partText[partText.size()-2]->sceneBoundingRect().center())/2.0;
		for(QList<PartViewText *>::iterator it=partText.begin();it!=partText.end();it++)
			if((*it)==partText.last())
			{
				(*it)->setTransformOriginPoint((*it)->boundingRect().center());
				(*it)->setRotation(-rotation());
			}
			else
			{
				(*it)->setTransformOriginPoint((*it)->mapFromScene(parameterTransformOriginPoint));
				(*it)->setRotation(-rotation());
			}
	}
	else if(partText.size()==1)
	{
		partText.last()->setTransformOriginPoint(partText.last()->boundingRect().center());
		partText.last()->setRotation(-rotation());
	}
}

QRectF PartView::boundingRect() const
{
    return childrenBoundingRect();
}

void PartView::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	QStyleOptionGraphicsItem op;
	op.initFrom(widget);
    if (option->state & QStyle::State_Selected)
	{
		op.state = QStyle::State_None;//去掉默认虚线框
		painter->setPen(QPen(Qt::red,0,Qt::DashLine));//绘制新虚线框
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(boundingRect().adjusted(-1,-1,1,1));
	}
	QGraphicsItemGroup::paint(painter,&op,widget);
}

void PartViewText::paint(QPainter *painter,const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	painter->setPen(QPen(QColor(255,255,255,0)));
	painter->setBrush(QBrush(backgroundColor));
	painter->drawRect(boundingRect());
	QGraphicsTextItem::paint(painter,option,widget);
}

void PartViewText::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	event->setScenePos(QPointF(snapToGrid(event->scenePos().x()),snapToGrid(event->scenePos().y())));
	event->setButtonDownScenePos(Qt::LeftButton,QPointF(snapToGrid(event->buttonDownScenePos(Qt::LeftButton).x()),snapToGrid(event->buttonDownScenePos(Qt::LeftButton).y())));
	QGraphicsTextItem::mouseMoveEvent(event);
	setPos(snapToGrid(pos().x()),snapToGrid(pos().y()));
}

void PartViewText::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsTextItem::mousePressEvent(event);
	if(index!=-1)
	{
		((MainWindow *)scene()->parent())->currentSelectPart=partView;
		((MainWindow *)scene()->parent())->setPartParameter();
	}
}

void PartViewText::focusOutEvent(QFocusEvent *event)
{
	QGraphicsTextItem::focusOutEvent(event);
	setTextInteractionFlags(Qt::NoTextInteraction);
	((GraphicsScene *)scene())->isEditingText=false;
}

void PartViewText::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
	QGraphicsTextItem::mouseDoubleClickEvent(event);
    setTextInteractionFlags(Qt::TextEditorInteraction);
	setFocus();
	((GraphicsScene *)scene())->isEditingText=true;
}
