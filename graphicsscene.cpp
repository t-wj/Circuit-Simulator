#include "graphicsscene.h"
#include "mainwindow.h"
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QGraphicsView>

QList<Wire *> GraphicsScene::connectedWires(QGraphicsItem *item)
{//查找与item相碰撞的连线，返回碰撞连线列表
	QList<QGraphicsItem *> collidingItem=collidingItems(item);
	QList<Wire *> connectedWire;
	QRectF tmpWire;
	for(QList<QGraphicsItem *>::iterator it=collidingItem.begin();it!=collidingItem.end();it++)
		if((*it)->type()==Wire::Type)
		{
			tmpWire=((Wire *)item)->sceneBoundingRect().adjusted(WIRE_WIDTH_TO_SELECT/2.0,WIRE_WIDTH_TO_SELECT/2.0,-WIRE_WIDTH_TO_SELECT/2.0,-WIRE_WIDTH_TO_SELECT/2.0);
			if(((Wire *)(*it))->sceneBoundingRect().contains(tmpWire.topLeft())||(*it)->sceneBoundingRect().contains(tmpWire.bottomRight()))
			{
				connectedWire.push_back((Wire *)(*it));
				continue;
			}
			tmpWire=((Wire *)(*it))->sceneBoundingRect().adjusted(WIRE_WIDTH_TO_SELECT/2.0,WIRE_WIDTH_TO_SELECT/2.0,-WIRE_WIDTH_TO_SELECT/2.0,-WIRE_WIDTH_TO_SELECT/2.0);
			if(((Wire *)item)->sceneBoundingRect().contains(tmpWire.topLeft())||((Wire *)item)->sceneBoundingRect().contains(tmpWire.bottomRight()))
			{
				connectedWire.push_back((Wire *)(*it));
				continue;
			}
		}
	return connectedWire;
}

void GraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsScene::mousePressEvent(event);
	//结束放置元件
	isPlacingPart=false;
	if(currentPlacingPartGroup!=NULL)
	{
		destroyItemGroup(currentPlacingPartGroup);
		currentPlacingPartGroup=NULL;
		currentPlacingParts.clear();
	}
	if(event->buttons()==Qt::LeftButton&&isDrawingWire)
	{
		++mousePressCountWhenDrawingWire;
		if(mousePressCountWhenDrawingWire==1)
		{//开始绘制连线
			wireBeginPoint=event->scenePos();
			wireDrawingMode=Unspecified;
			currentDrawingHorizontalWire=new Wire;
			currentDrawingVerticalWire=new Wire;
			addItem(currentDrawingHorizontalWire);
			addItem(currentDrawingVerticalWire);
		}
		else if(mousePressCountWhenDrawingWire>1)
		{
			wireEndPoint=event->scenePos();
			//移除多余连线
			if(snapToGrid(wireEndPoint.x())==snapToGrid(wireBeginPoint.x()))
			{
				removeItem(currentDrawingHorizontalWire);
				delete currentDrawingHorizontalWire;
			}
			if(snapToGrid(wireEndPoint.y())==snapToGrid(wireBeginPoint.y()))
			{
				removeItem(currentDrawingVerticalWire);
				delete currentDrawingVerticalWire;
			}
			QList<QGraphicsItem *> tmpItem=items();
			for(QList<QGraphicsItem *>::iterator it=tmpItem.begin();it!=tmpItem.end();it++)
				if((*it)!=currentDrawingHorizontalWire&&(*it)!=currentDrawingVerticalWire&&(*it)->type()==Wire::Type&&(*it)->sceneBoundingRect().contains(wireEndPoint))
				{//若当前位置已在某条连线范围内，则停止当前绘制，进入新的连线绘制
					//重置变量
					mousePressCountWhenDrawingWire=0;
					currentDrawingHorizontalWire=NULL;
					currentDrawingVerticalWire=NULL;
					//恢复QGraphicsItem属性，移除所有交点标识
					QList<QGraphicsItem *> tmpItems=items();
					for(QList<QGraphicsItem *>::iterator it=tmpItems.begin();it!=tmpItems.end();it++)
						if((*it)->type()==QGraphicsEllipseItem::Type)
						{
							removeItem((*it));
							delete *it;
						}
						else if((*it)->type()!=QGraphicsPixmapItem::Type)
						{
							(*it)->setAcceptDrops(true);
							(*it)->setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsFocusable);
						}
					tmpItems=items();
					for(QList<QGraphicsItem *>::iterator it=tmpItems.begin();it!=tmpItems.end();it++)
						if((*it)->type()==PartView::Type)
							for(QList<Wire *>::iterator wit=((PartView *)(*it))->partPin.begin();wit!=((PartView *)(*it))->partPin.end();wit++)
							{
								(*wit)->setAcceptDrops(false);
								(*wit)->setFlag(QGraphicsItem::ItemIsSelectable,false);
								(*wit)->setFlag(QGraphicsItem::ItemIsMovable,false);
								(*wit)->setFlag(QGraphicsItem::ItemIsFocusable,false);
							}
					return;
				}
			//否则继续当前绘制
			mousePressCountWhenDrawingWire=1;
			wireBeginPoint=event->scenePos();
			wireDrawingMode=Unspecified;
			currentDrawingHorizontalWire=new Wire;
			currentDrawingVerticalWire=new Wire;
			addItem(currentDrawingHorizontalWire);
			addItem(currentDrawingVerticalWire);
		}
	}
}
void GraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsScene::mouseMoveEvent(event);
	QList<QGraphicsItem *> tmpItems=items();
	QList<QGraphicsItem *>::iterator it;
	if(!(event->buttons()&Qt::LeftButton))
		for(it=tmpItems.begin();it!=tmpItems.end();it++)//移除所有旧有交点标识
			if((*it)->type()==QGraphicsEllipseItem::Type)
			{
				removeItem((*it));
				delete *it;
			}

	if(isPlacingPart)
	{
		if(!currentPlacingParts.isEmpty())
		{
			//放置元件
			QPointF snapPos=snapPoint();
			currentPlacingPartGroup->setPos(snapToGrid(event->scenePos().x())-snapPos.x(),snapToGrid(event->scenePos().y())-snapPos.y());

			//重新标记交点
			QList<QGraphicsItem *> tmpCollidingItems=items();
			QList<QGraphicsItem *>::iterator _it;
			tmpItems=items();
			QList<Wire *>::iterator wit;
			//去除tmpCollidingItems中多余的元素
			for(it=currentPlacingParts.begin();it!=currentPlacingParts.end();it++)
				if((*it)->type()==Wire::Type)
					tmpCollidingItems.removeAll(*it);
				else if((*it)->type()==PartView::Type)
					for(wit=((PartView *)(*it))->partPin.begin();wit!=((PartView *)(*it))->partPin.end();wit++)
						tmpCollidingItems.removeAll(*wit);
			for(it=tmpCollidingItems.begin();it!=tmpCollidingItems.end();)
				if((*it)->type()!=Wire::Type)
					it=tmpCollidingItems.erase(it);
				else
					it++;
			//添加交点标识
			for(it=currentPlacingParts.begin();it!=currentPlacingParts.end();it++)
				if((*it)->type()==Wire::Type)
					for(_it=tmpCollidingItems.begin();_it!=tmpCollidingItems.end();_it++)
						addCollidingSign(*it,*_it);
				else if((*it)->type()==PartView::Type)
					for(wit=((PartView *)(*it))->partPin.begin();wit!=((PartView *)(*it))->partPin.end();wit++)
						for(_it=tmpCollidingItems.begin();_it!=tmpCollidingItems.end();_it++)
							addCollidingSign(*wit,*_it);
		}
		else
		{
			isPlacingPart=false;
			currentPlacingPartGroup=NULL;
		}
	}
	else if(isDrawingWire)
	{//绘制连线
		if(mousePressCountWhenDrawingWire==0)
		{
			views().first()->viewport()->setCursor(Qt::CrossCursor);
			//屏蔽元件属性
			tmpItems=items();
			for(it=tmpItems.begin();it!=tmpItems.end();it++)
			{
				(*it)->setAcceptDrops(false);
				(*it)->setFlag(QGraphicsItem::ItemIsSelectable,false);
				(*it)->setFlag(QGraphicsItem::ItemIsMovable,false);
				(*it)->setFlag(QGraphicsItem::ItemIsFocusable,false);
			}
		}
		else if(mousePressCountWhenDrawingWire==1)
		{
			if(snapToGrid(event->scenePos().x())==snapToGrid(wireBeginPoint.x())||snapToGrid(event->scenePos().y())==snapToGrid(wireBeginPoint.y()))
				wireDrawingMode=Unspecified;
			switch(wireDrawingMode)
			{
			case Unspecified:
				//判断绘制类型
				wireEndPoint=event->scenePos();
				if(fabs((double)(wireEndPoint.x()-wireBeginPoint.x()))>fabs((double)(wireEndPoint.y()-wireBeginPoint.y())))
					wireDrawingMode=Horizontal;
				else
					wireDrawingMode=Vertical;
				//继续绘制
			case Horizontal:
				wireBeginPoint=QPointF(snapToGrid(wireBeginPoint.x()),snapToGrid(wireBeginPoint.y()));
				wireEndPoint=QPointF(snapToGrid(event->scenePos().x()),snapToGrid(event->scenePos().y()));
				currentDrawingHorizontalWire->setLine(wireBeginPoint.x(),wireBeginPoint.y(),wireEndPoint.x(),wireBeginPoint.y());
				currentDrawingVerticalWire->setLine(wireEndPoint.x(),wireBeginPoint.y(),wireEndPoint.x(),wireEndPoint.y());
				break;
			case Vertical:
				wireBeginPoint=QPointF(snapToGrid(wireBeginPoint.x()),snapToGrid(wireBeginPoint.y()));
				wireEndPoint=QPointF(snapToGrid(event->scenePos().x()),snapToGrid(event->scenePos().y()));
				currentDrawingHorizontalWire->setLine(wireBeginPoint.x(),wireBeginPoint.y(),wireBeginPoint.x(),wireEndPoint.y());
				currentDrawingVerticalWire->setLine(wireBeginPoint.x(),wireEndPoint.y(),wireEndPoint.x(),wireEndPoint.y());
				break;
			}

			//绘制交点标识
			tmpItems.clear();
			tmpItems=collidingItems(currentDrawingHorizontalWire)+collidingItems(currentDrawingVerticalWire);
			for(it=tmpItems.begin();it!=tmpItems.end();)
				if((*it)->type()!=Wire::Type||(*it)==currentDrawingHorizontalWire||(*it)==currentDrawingVerticalWire)
					it=tmpItems.erase(it);
				else
					it++;

			for(it=tmpItems.begin();it!=tmpItems.end();it++)
			{
				addCollidingSign(currentDrawingHorizontalWire,*it);
				addCollidingSign(currentDrawingVerticalWire,*it);
			}
		}
	}
}

QPointF GraphicsScene::snapPoint()
{//根据currentPlacingParts，选择一个部件，根据该部件位置返回一个供对齐的坐标
	QGraphicsItem *snapItem=currentPlacingParts.first();
	QPointF snapPos;
	if(snapItem->type()==PartView::Type)
	{
		QList<QGraphicsItem *> tmp=((PartView *)snapItem)->childItems();
		for(QList<QGraphicsItem *>::iterator it=tmp.begin();it!=tmp.end();it++)
			if((*it)->type()==QGraphicsPixmapItem::Type)
			{
				snapPos=currentPlacingPartGroup->mapFromScene((*it)->sceneBoundingRect().center());
				break;
			}
	}
	else if(snapItem->type()==Wire::Type)
		snapPos=currentPlacingPartGroup->mapFromScene(snapItem->sceneBoundingRect().adjusted
		(WIRE_WIDTH_TO_SELECT/2.0,WIRE_WIDTH_TO_SELECT/2.0,-WIRE_WIDTH_TO_SELECT/2.0,-WIRE_WIDTH_TO_SELECT/2.0).bottomRight());
	return snapPos;
}

void GraphicsScene::addCollidingSign(QGraphicsItem *item1,QGraphicsItem *item2)
{//添加交点标识
	QRectF tmpWire;
	const QColor ellipseColor(255,0,0,127);
	tmpWire=item2->sceneBoundingRect().adjusted(WIRE_WIDTH_TO_SELECT/2.0,WIRE_WIDTH_TO_SELECT/2.0,-WIRE_WIDTH_TO_SELECT/2.0,-WIRE_WIDTH_TO_SELECT/2.0);
	if(item1->sceneBoundingRect().contains(tmpWire.topLeft()))
	{
		addEllipse(tmpWire.topLeft().x()-COLLIDING_CIRCLE_RADIUS,tmpWire.topLeft().y()-COLLIDING_CIRCLE_RADIUS,
			2*COLLIDING_CIRCLE_RADIUS,2*COLLIDING_CIRCLE_RADIUS,QPen(ellipseColor),QBrush(ellipseColor));
		return;
	}
	if(item1->sceneBoundingRect().contains(tmpWire.bottomRight()))
	{
		addEllipse(tmpWire.bottomRight().x()-COLLIDING_CIRCLE_RADIUS,tmpWire.bottomRight().y()-COLLIDING_CIRCLE_RADIUS,
			2*COLLIDING_CIRCLE_RADIUS,2*COLLIDING_CIRCLE_RADIUS,QPen(ellipseColor),QBrush(ellipseColor));
		return;
	}
	tmpWire=item1->sceneBoundingRect().adjusted(WIRE_WIDTH_TO_SELECT/2.0,WIRE_WIDTH_TO_SELECT/2.0,-WIRE_WIDTH_TO_SELECT/2.0,-WIRE_WIDTH_TO_SELECT/2.0);
	if(item2->sceneBoundingRect().contains(tmpWire.topLeft()))
	{
		addEllipse(tmpWire.topLeft().x()-COLLIDING_CIRCLE_RADIUS,tmpWire.topLeft().y()-COLLIDING_CIRCLE_RADIUS,
			2*COLLIDING_CIRCLE_RADIUS,2*COLLIDING_CIRCLE_RADIUS,QPen(ellipseColor),QBrush(ellipseColor));
		return;
	}
	if(item2->sceneBoundingRect().contains(tmpWire.bottomRight()))
	{
		addEllipse(tmpWire.bottomRight().x()-COLLIDING_CIRCLE_RADIUS,tmpWire.bottomRight().y()-COLLIDING_CIRCLE_RADIUS,
			2*COLLIDING_CIRCLE_RADIUS,2*COLLIDING_CIRCLE_RADIUS,QPen(ellipseColor),QBrush(ellipseColor));
		return;
	}
}

void GraphicsScene::keyPressEvent(QKeyEvent *event)
{
	if(isEditingText&&(event->key()==Qt::Key_Enter||event->key()==Qt::Key_Return||event->key()==QKeySequence::InsertParagraphSeparator))
		clearFocus();
	if((event->key()==Qt::Key_Delete||event->key()==Qt::Key_Backspace)&&!selectedItems().isEmpty()&&!isEditingText)
	{
		QList<QGraphicsItem *> tmpSelectedItems;
		((MainWindow *)parent())->currentSelectPart=NULL;
		bool loop=true;
		while(loop)
		{
			loop=false;
			tmpSelectedItems=selectedItems();
			for(QList<QGraphicsItem *>::iterator it=tmpSelectedItems.begin();it!=tmpSelectedItems.end();it++)
				if((*it)->type()==PartView::Type)
				{
					circuitParts.remove(((PartView *)(*it))->part);
					delete ((PartView *)(*it))->part;
					removeItem(*it);
					delete *it;
					loop=true;
					break;
				}
		}
		tmpSelectedItems=selectedItems();
		for(QList<QGraphicsItem *>::iterator it=tmpSelectedItems.begin();it!=tmpSelectedItems.end();it++)
			if((*it)->type()==Wire::Type)
			{
				removeItem(*it);
				delete *it;
			}
	}
	if(event->key()==Qt::Key_Escape&&isDrawingWire)
	{
		//移除正在绘制的连线、重置变量
		isDrawingWire=false;
		mousePressCountWhenDrawingWire=0;
		if(currentDrawingHorizontalWire!=NULL)
		{
			removeItem(currentDrawingHorizontalWire);
			delete currentDrawingHorizontalWire;
		}
		if(currentDrawingVerticalWire!=NULL)
		{
			removeItem(currentDrawingVerticalWire);
			delete currentDrawingVerticalWire;
		}
		currentDrawingHorizontalWire=NULL;
		currentDrawingVerticalWire=NULL;
		views().first()->viewport()->unsetCursor();
		//恢复QGraphicsItem属性，移除所有交点标识
		QList<QGraphicsItem *> tmpItems=items();
		for(QList<QGraphicsItem *>::iterator it=tmpItems.begin();it!=tmpItems.end();it++)
			if((*it)->type()==QGraphicsEllipseItem::Type)
			{
				removeItem((*it));
				delete *it;
			}
			else if((*it)->type()!=QGraphicsPixmapItem::Type)
			{
				(*it)->setAcceptDrops(true);
				(*it)->setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsFocusable);
			}
		tmpItems=items();
		for(QList<QGraphicsItem *>::iterator it=tmpItems.begin();it!=tmpItems.end();it++)
			if((*it)->type()==PartView::Type)
				for(QList<Wire *>::iterator wit=((PartView *)(*it))->partPin.begin();wit!=((PartView *)(*it))->partPin.end();wit++)
				{
					(*wit)->setAcceptDrops(false);
					(*wit)->setFlag(QGraphicsItem::ItemIsSelectable,false);
					(*wit)->setFlag(QGraphicsItem::ItemIsMovable,false);
					(*wit)->setFlag(QGraphicsItem::ItemIsFocusable,false);
				}
	}
	QGraphicsScene::keyPressEvent(event);
}