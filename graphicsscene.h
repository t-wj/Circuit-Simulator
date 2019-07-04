#ifndef GRAPHICSSCENE_H
#define GRAPHICSSCENE_H

#include <QGraphicsScene>
#include "element.h"
#include "graphicsitem.h"
#include "flist.h"

class GraphicsScene : public QGraphicsScene
{
public:
	bool isDrawingWire;
	bool isPlacingPart;
	bool isEditingText;
	fList<Element *> circuitParts;//原理图中的元件对应的Element对象列表
	QList<QGraphicsItem *> currentPlacingParts;
	QGraphicsItemGroup *currentPlacingPartGroup;
	int mousePressCountWhenDrawingWire;
	QList<Wire *> connectedWires(QGraphicsItem *item);
	inline QPointF snapPoint();
    void addCollidingSign(QGraphicsItem *item1,QGraphicsItem *item2);
	void keyPressEvent(QKeyEvent *event);
	GraphicsScene(QObject *parent = 0):QGraphicsScene(parent),isEditingText(false),
		isPlacingPart(false),currentPlacingPartGroup(NULL),currentDrawingHorizontalWire(NULL),currentDrawingVerticalWire(NULL),
		isDrawingWire(false),mousePressCountWhenDrawingWire(0){}
	~GraphicsScene()
	{
		clear();//清除原理图上的所有部件
		for(fList<Element *>::iterator it=circuitParts.begin();it!=circuitParts.end();++it)
			delete *it;//清除circuitParts
		circuitParts.clear();
	}

private:
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	enum wireDrawingModeName{Unspecified,Horizontal,Vertical} wireDrawingMode;
	QPointF wireBeginPoint;
	QPointF wireEndPoint;
	Wire *currentDrawingHorizontalWire;
	Wire *currentDrawingVerticalWire;
};

#endif // GRAPHICSSCENE_H
