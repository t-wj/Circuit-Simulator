#ifndef GRAPHICSITEM_H
#define GRAPHICSITEM_H

#include <QGraphicsItem>
#include <QTextDocument>

class Element;
class PartViewText;
class Wire : public QGraphicsLineItem
{
public:
	int wireNo;
	int type() const {return Type;}
	enum {Type=QGraphicsItem::UserType+1};
	QPainterPath shape() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	Wire(qreal _x1=0.0,qreal _y1=0.0,qreal _x2=0.0,qreal _y2=0.0)
		:QGraphicsLineItem(_x1,_y1,_x2,_y2),wireNo(++autoWireNo)
	{
		setFlags(ItemIsSelectable|ItemIsMovable);
	}

private:
	static int autoWireNo;
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
};

class PartView : public QGraphicsItemGroup
{
public:
	Element *part;//该对象所对应的Element抽象元件对象
    QList<Wire *> partPin;//元件管脚，从0开始，由元件视图的正上方或左侧起进行编号，以逆时针为序
	QList<PartViewText *> partText;//元件标注（包括元件名、元件参数）
	QList<PartViewText *> simulationText;//时域仿真中的仿真结果标记
	PartView(Element *_part);
	int type() const {return Type;}
	enum {Type=QGraphicsItem::UserType+2};
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void initPin();
	void initViewText();
	void rotatePart();
	QRectF boundingRect() const;

private:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
};

class PartViewText : public QGraphicsTextItem
{
	Q_OBJECT

public:
	PartViewText(QString _text,PartView *_partView,int _index=-1,QColor _backgroundColor=QColor(255,255,255,0))
		:QGraphicsTextItem(_text),partView(_partView),index(_index),backgroundColor(_backgroundColor),oldValue("")
	{
		setFlags(ItemIsSelectable|ItemIsMovable|ItemIsFocusable);
		setTextInteractionFlags(Qt::NoTextInteraction);
		connect(document(),SIGNAL(contentsChanged()),this,SLOT(parameterValueChangedSlot()));
	}
	PartView *partView;
	int index;
	QString oldValue;
	int type() const {return Type;}
	enum {Type=QGraphicsItem::UserType+3};

public slots:
	void parameterValueChangedSlot()
	{
		emit parameterValueChanged(index,(oldValue=toPlainText()));
	}

signals:
	void parameterValueChanged(int pinIndex,QString value);

private:
	QColor backgroundColor;
	void paint(QPainter *painter,const QStyleOptionGraphicsItem *option,QWidget *widget);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event);
	void focusOutEvent(QFocusEvent *event);
};

#endif // GRAPHICSITEM_H
