#include "element.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingdialog.h"
#include "ui_simulationsetting.h"
#include "resultdialog.h"
#include "simulate.h"
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>
#include <QGraphicsScene>
#include <QComboBox>
#include <QMimeData>
#include <QClipboard>
#include <QStyleFactory>
#include <fstream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), currentSelectPart(NULL), sceneOfSchematic(NULL), isSaved(true), 
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	sceneOfSchematic=newGraphicsScene();
	ui->tableToSetPart->verticalHeader()->setFixedWidth(180);
	ui->mainToolBar->setStyle(QStyleFactory::create("Fusion"));
	//创建元件列表
	Element::mapInit();
	for(map<string,string>::iterator it=Element::viewNameToType.begin();it!=Element::viewNameToType.end();it++)
        ui->partList->addItem(QString::fromLocal8Bit((*it).first.c_str()));
	//设置背景图片
	QPalette paletteOfEdit;
	paletteOfEdit.setBrush(QPalette::Window, QBrush(QPixmap(":/background/background")));
	ui->edit->setAutoFillBackground(true);
	ui->edit->setPalette(paletteOfEdit);
	//设置ui->viewOfSchematic的右键菜单
	ui->viewOfSchematic->addAction(ui->cutAction);
	ui->viewOfSchematic->addAction(ui->copyAction);
	ui->viewOfSchematic->addAction(ui->pasteAction);
	ui->viewOfSchematic->addAction(ui->selectAllAction);
	ui->viewOfSchematic->addAction(ui->deleteAction);

	statusBar()->showMessage(QStringLiteral("双击元件列表中的元件，以放置元件"));
}

GraphicsScene *MainWindow::newGraphicsScene()
{//新建graphicsScene对象，并与ui->viewOfSchematic关联
	GraphicsScene *tmpScene=new GraphicsScene(this);
	tmpScene->setSceneRect(QRectF(0,0,1600,1600));
	ui->viewOfSchematic->setScene(tmpScene);//将电路图场景与视图相关联
	ui->viewOfSchematic->centerOn(0.0,0.0);
	QObject::connect(tmpScene,SIGNAL(selectionChanged()),this,SLOT(setSelectedPartParameter()));//连接信号/槽
	//绘制网格背景
	QPolygonF polygon;
	polygon<<QPointF(0,0)<<QPointF(GRID_WIDTH,0);
	QPixmap pixmap(GRID_WIDTH,GRID_WIDTH);
	pixmap.fill(Qt::white);
	QPainter painter(&pixmap);
	QVector<qreal> dashes;
	qreal space=GRID_WIDTH;
	dashes<<1<<space;
	QPen pen(Qt::gray,1);
	pen.setDashPattern(dashes);  
	pen.setWidth(1);    
	painter.setPen(pen);  
	painter.translate(0, 0);  
	painter.drawPolyline(polygon); 
	tmpScene->setBackgroundBrush(pixmap);
	//初始化表格
	ui->tableToSetPart->clear();
	ui->tableToSetPart->setRowCount(0);
	ui->labelToViewPart->setPixmap(QPixmap(""));
	ui->labelToViewPart->show();
	ui->labelToViewPart->setText("View");

	return tmpScene;
}

MainWindow::~MainWindow()
{
	delete sceneOfSchematic;
    delete ui;
}

void MainWindow::openSchematic()
{
	if(isSaved||saveOnCloseQuestion())
	{
		QString tmpFileName=QFileDialog::getOpenFileName(this,QStringLiteral("选择原理图文件"),"./",QStringLiteral("原理图文件(*.sch)"));
		if(tmpFileName.isNull())
			return;
		else if(tmpFileName.split(".").last()!="sch")
		{
			QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("打开失败：文件后缀应为“.sch”"));
			return;
		}
		else
			fileName=tmpFileName;
        ifstream infile(fileName.toStdString(),ios::in);
		if(!infile)
		{
			QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("打开文件失败"));
			return;
		}
		else
			setWindowTitle(fileName.split("/").last()+QStringLiteral(" - 简易电路仿真器"));
		
		delete sceneOfSchematic;
		sceneOfSchematic=newGraphicsScene();
		sCurrentSelectPart.clear();
		currentSelectPart=NULL;
		isSaved=true;
		inputItems(infile,false);
		infile.close();
	}
}

QList<QGraphicsItem *> MainWindow::inputItems(istream &input,bool autoNo)
{//从input流中读取元件数据至sceneOfSchematic->circuitParts；autoNo决定新建元件是否自动编号
	qreal x1,y1,x2,y2;
	string tmpType,tmpName;
	Element *tmpPart;
	Wire *tmpWire;
	PartView *tmpPartView;
	QList<QGraphicsItem *> tmpItems;
	while(input>>tmpType)
	{
		if(tmpType=="WIRE")
		{
			input>>x1>>y1>>x2>>y2;
			tmpWire=new Wire(x1,y1,x2,y2);
			sceneOfSchematic->addItem(tmpWire);
			tmpItems.push_back(tmpWire);
		}
		else if(tmpType=="STATICN"&&!autoNo)
			Element::setStaticNo(input);
		else if((tmpPart=Element::newElement(tmpType))!=NULL)
		{//如果对象创建成功，则向原理图上添加元件
			if(!autoNo)
				input>>tmpPart->name;
			else
				input>>tmpName;
			input>>*tmpPart;
			sceneOfSchematic->circuitParts.push_front(tmpPart);
			tmpPartView=newPartView(tmpPart);
			input>>x2>>x1>>y1;
			tmpPartView->setPos(x1,y1);
			for(int i=0;i<-x2/90.0;i++)
				tmpPartView->rotatePart();
			for(QList<PartViewText *>::iterator it=tmpPartView->partText.begin();it!=tmpPartView->partText.end();it++)
			{
				input>>x1>>y1;
				(*it)->setPos(x1,y1);
			}
			tmpItems.push_back(tmpPartView);
		}
		else//否则跳过该行
			getline(input,tmpType);
	}
	return tmpItems;
}

void MainWindow::searchPart(QString partType)
{//搜索partType所对应的元件，并将元件列表焦点设置为该元件
	if(!ui->partList->findItems(partType,Qt::MatchContains).isEmpty())
		ui->partList->setCurrentItem(ui->partList->findItems(partType,Qt::MatchContains).first());
}

void MainWindow::viewPartImage()
{//显示元件预览图
	if(!ui->partList->selectedItems().isEmpty())
	{
		QListWidgetItem *part=ui->partList->selectedItems().first();
		ui->tableToSetPart->clear();
		ui->tableToSetPart->setRowCount(0);
        ui->labelToViewPart->setPixmap(Element::viewImagePixmap(Element::viewNameToType[part->text().toStdString()]));
		ui->labelToViewPart->show();
	}
	else
	{
		currentSelectPart=NULL;
		ui->tableToSetPart->clear();
		ui->tableToSetPart->setRowCount(0);
		ui->labelToViewPart->setPixmap(QPixmap(""));
		ui->labelToViewPart->show();
		ui->labelToViewPart->setText("View");
	}

}

void MainWindow::initPart(QListWidgetItem *part)
{//初始化新元件
	Element *eCurrentSelectPart;
    if(sceneOfSchematic->isDrawingWire==false&&(eCurrentSelectPart=Element::newElement(Element::viewNameToType[string(part->text().toLocal8Bit())]))!=NULL)
	{
		isSaved=false;//设置保存状态
		setWindowTitle(windowTitle().split(QRegExp("[* ]")).first()+QStringLiteral("* - 简易电路仿真器"));
		sceneOfSchematic->circuitParts.push_front(eCurrentSelectPart);
		sceneOfSchematic->currentPlacingParts.clear();
		currentSelectPart=newPartView(eCurrentSelectPart);
		sceneOfSchematic->currentPlacingParts.push_back(currentSelectPart);
		sceneOfSchematic->isPlacingPart=true;
		sceneOfSchematic->currentPlacingPartGroup
			=sceneOfSchematic->createItemGroup(sceneOfSchematic->currentPlacingParts);
		QCursor::setPos(ui->viewOfSchematic->mapToGlobal(QPoint(ui->viewOfSchematic->width()/2, ui->viewOfSchematic->height()/2)));
        setPartParameter();
	}
}

PartView *MainWindow::newPartView(Element *part)
{
	PartView *tmpPartView=new PartView(part);
	sceneOfSchematic->addItem(tmpPartView);
	//初始化元件图形
	QGraphicsPixmapItem *pixmap=new QGraphicsPixmapItem;
	pixmap->setFlag(QGraphicsItem::ItemIsSelectable,false);
	pixmap->setFlag(QGraphicsItem::ItemIsMovable,false);
	tmpPartView->addToGroup(pixmap);
	pixmap->setPixmap(Element::imagePixmap(part->type));
	pixmap->show();
	//初始化元件管脚
	tmpPartView->initPin();
	//初始化元件标注
	tmpPartView->initViewText();
	for(int i=0;i<tmpPartView->partText.size();i++)
	{
		connect(tmpPartView->partText[i],SIGNAL(parameterValueChanged(int,QString)),this,SLOT(setPartParameter(int,QString)));
		tmpPartView->addToGroup(tmpPartView->partText[i]);
	}
	//设置元件初始位置及旋转角度
	tmpPartView->setPos(ui->viewOfSchematic->mapToScene(snapToGrid(ui->viewOfSchematic->width()/2 - pixmap->boundingRect().width()/2), 
		snapToGrid(ui->viewOfSchematic->height()/2 - pixmap->boundingRect().height()/2)));
	if(tmpPartView->part->type[0]=='V'||tmpPartView->part->type[0]=='I')
		tmpPartView->rotatePart();//电源元件，依惯例旋转90度
	return tmpPartView;
}

void MainWindow::setSelectedPartParameter()
{//设置选定元件参数表格
	isSaved=false;//设置保存状态
	setWindowTitle(windowTitle().split(QRegExp("[* ]")).first()+QStringLiteral("* - 简易电路仿真器"));
	if(sceneOfSchematic->selectedItems().size()==1&&sceneOfSchematic->selectedItems().first()->type()==PartView::Type)
	{
		if(currentSelectPart!=NULL)
			currentSelectPart->setZValue(0.0);
		currentSelectPart=(PartView *)sceneOfSchematic->selectedItems().first();
		currentSelectPart->setZValue(1.0);//使当前选定元件位于顶层
		setPartParameter();
	}
	else
	{
		currentSelectPart=NULL;
		ui->tableToSetPart->clear();
		ui->tableToSetPart->setRowCount(0);
		ui->labelToViewPart->setPixmap(QPixmap(""));
		ui->labelToViewPart->show();
		ui->labelToViewPart->setText("View");
	}
}

void MainWindow::setPartParameter(int index,QString value)
{//设置参数表中index对应的数据
	if(currentSelectPart!=NULL&&currentSelectPart->type()==PartView::Type&&index!=-1)
	{
        if(index<currentSelectPart->partText.size()-1)
            ui->tableToSetPart->setItem(index+1,0,new QTableWidgetItem(value));
		else
			ui->tableToSetPart->setItem(0,0,new QTableWidgetItem(value));
	}
}

void MainWindow::savePartParameter(int row, int column)
{//保存元件属性至Element对象，当参数表格数据被修改时触发
	if(currentSelectPart!=NULL&&currentSelectPart->type()==PartView::Type)
		if(row==0)
		{//元件名
            currentSelectPart->part->name=ui->tableToSetPart->item(row,column)->text().toStdString();
			if(currentSelectPart->partText.last()->oldValue!=ui->tableToSetPart->item(row,column)->text())//防止进入死循环
			{
				currentSelectPart->partText.last()->setPlainText(ui->tableToSetPart->item(row,column)->text());
				currentSelectPart->partText.last()->oldValue=ui->tableToSetPart->item(row,column)->text();
			}
		}
		else
		{//元件参数
			string t=ui->tableToSetPart->item(row,column)->text().toStdString();
			currentSelectPart->part->setParameter(row-1,etof(ui->tableToSetPart->item(row,column)->text().toStdString()));
			if(currentSelectPart->partText.size()>row&&currentSelectPart->partText[row-1]->oldValue!=ui->tableToSetPart->item(row,column)->text())//防止进入死循环
			{
				currentSelectPart->partText[row-1]->setPlainText(ui->tableToSetPart->item(row,column)->text());
				currentSelectPart->partText[row-1]->oldValue=ui->tableToSetPart->item(row,column)->text();
			}
            if(currentSelectPart->part->type=="C"&&row==2)
                ((Capacitor *)(currentSelectPart->part))->setInitVoltage=(ui->tableToSetPart->item(2,0)->checkState()==Qt::Checked);
            else if(currentSelectPart->part->type=="L"&&row==2)
                ((Inductor *)(currentSelectPart->part))->setInitCurrent=(ui->tableToSetPart->item(2,0)->checkState()==Qt::Checked);
		}
}

void MainWindow::setPartParameter()
{//设置参数表格及预览图
	if(currentSelectPart!=NULL&&currentSelectPart->type()==PartView::Type)
	{
		//设置参数表
		ui->tableToSetPart->clear();
		ui->tableToSetPart->setRowCount(currentSelectPart->part->parameterNum+1);
		ui->tableToSetPart->setVerticalHeaderItem(0,new QTableWidgetItem(QStringLiteral("元件名")));
        ui->tableToSetPart->setItem(0,0,new QTableWidgetItem(QString::fromLocal8Bit(currentSelectPart->part->name.c_str())));
        QTableWidgetItem *tmpItem;
		for(int i=0;i<currentSelectPart->part->parameterNum;i++)
		{
            ui->tableToSetPart->setVerticalHeaderItem(i+1,new QTableWidgetItem(QString::fromLocal8Bit(currentSelectPart->part->showParameterViewName(i))));
            tmpItem=new QTableWidgetItem(ftoe((currentSelectPart->part->showParameter(i))).c_str());
            if(currentSelectPart->part->type=="C"&&i==1)
            {
                tmpItem->setFlags(tmpItem->flags()|Qt::ItemIsUserCheckable);
                tmpItem->setCheckState(((Capacitor *)(currentSelectPart->part))->setInitVoltage?Qt::Checked:Qt::Unchecked);
            }
            else if(currentSelectPart->part->type=="L"&&i==1)
            {
                tmpItem->setFlags(tmpItem->flags()|Qt::ItemIsUserCheckable);
                tmpItem->setCheckState(((Inductor *)(currentSelectPart->part))->setInitCurrent?Qt::Checked:Qt::Unchecked);
            }
            ui->tableToSetPart->setItem(i+1,0,tmpItem);
		}
		//设置元件预览图片
		ui->labelToViewPart->setPixmap(Element::viewImagePixmap(currentSelectPart->part->type));
		ui->labelToViewPart->show();
        setEditState(2,0);
	}
}

void MainWindow::setEditState(int row,int column)
{
    if(currentSelectPart->part->type=="C"&&row==2)
    {
        if(((Capacitor *)(currentSelectPart->part))->setInitVoltage)
        {
            ui->tableToSetPart->item(2,0)->setFlags(ui->tableToSetPart->item(2,0)->flags()|Qt::ItemIsEditable);
            ui->tableToSetPart->item(2,0)->setTextColor(Qt::black);
        }
        else
        {
            ui->tableToSetPart->item(2,0)->setFlags(ui->tableToSetPart->item(2,0)->flags()&~Qt::ItemIsEditable);
            ui->tableToSetPart->item(2,0)->setTextColor(Qt::gray);
        }
    }
    else if(currentSelectPart->part->type=="L"&&row==2)
    {
        if(((Inductor *)(currentSelectPart->part))->setInitCurrent)
        {
            ui->tableToSetPart->item(2,0)->setFlags(ui->tableToSetPart->item(2,0)->flags()|Qt::ItemIsEditable);
            ui->tableToSetPart->item(2,0)->setTextColor(Qt::black);
        }
        else
        {
            ui->tableToSetPart->item(2,0)->setFlags(ui->tableToSetPart->item(2,0)->flags()&~Qt::ItemIsEditable);
            ui->tableToSetPart->item(2,0)->setTextColor(Qt::gray);
        }
    }
}

void MainWindow::drawWire()
{//进入连线绘制模式
	if(sceneOfSchematic->isPlacingPart==false)
	{
		sceneOfSchematic->isDrawingWire=true;
		isSaved=false;//设置保存状态
		setWindowTitle(windowTitle().split(QRegExp("[* ]")).first()+QStringLiteral("* - 简易电路仿真器"));
	}
}

void MainWindow::rotatePart()
{//旋转元件
	if(currentSelectPart!=NULL&&currentSelectPart->type()==PartView::Type)
		currentSelectPart->rotatePart();
}

bool MainWindow::saveNetlist()
{
	statusBar()->showMessage(QStringLiteral("正在生成网表文件……"));
	QList<QGraphicsItem *> tmpItems=sceneOfSchematic->items();
	QList<Wire *> connectedItem;
	QList<QGraphicsItem *>::iterator it;
	QList<QGraphicsItem *>::iterator _it;
	int i;

	//检测是否有同名元件，若有，则返回错误
	set<string> partName;
	for(it=tmpItems.begin();it!=tmpItems.end();it++)
		if((*it)->type()==PartView::Type)
			if(partName.find(((PartView *)(*it))->part->name)!=partName.end())
			{
				QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("网表文件生成失败：存在同名元件"));
				return false;
			}
			else
				partName.insert(((PartView *)(*it))->part->name);

	//对当前原理图上的连线及元件管脚进行重新编号，以避免之前原理图对当前图的干扰
	for(i=1,it=tmpItems.begin();it!=tmpItems.end();it++)
		if((*it)->type()==Wire::Type)
			((Wire *)(*it))->wireNo=i++;
		else if((*it)->type()==PartView::Type)
			for(int i=0;i<((PartView *)(*it))->part->pinNum;i++)
				((PartView *)(*it))->part->pin[i]=-2;
	//将所有与当前线相连连线对应编号相同的连线设为与当前线相同的编号
	for(it=tmpItems.begin();it!=tmpItems.end();it++)
		if((*it)->type()==Wire::Type)
		{
			connectedItem=sceneOfSchematic->connectedWires((*it));
			for(QList<Wire *>::iterator cit=connectedItem.begin();cit!=connectedItem.end();cit++)
			{
				int tmp=(*cit)->wireNo;
				for(_it=tmpItems.begin();_it!=tmpItems.end();_it++)
					if((*_it)->type()==Wire::Type&&((Wire *)(*_it))->wireNo==tmp)
						((Wire *)(*_it))->wireNo=((Wire *)(*it))->wireNo;
			}
		}
	//保存地节点编号
	set<int> GNDNode;
	for(it=tmpItems.begin();it!=tmpItems.end();it++)
		if((*it)->type()==PartView::Type&&((PartView *)(*it))->part->type=="GND")
			GNDNode.insert(((PartView *)(*it))->partPin.first()->wireNo);
	if(GNDNode.empty())
	{//若无地节点，则报错
		QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("网表文件生成失败：缺少地节点"));
		return false;
	}
	//将接地的节点编号设为0，并将其余节点从1开始重新编号
	map<int,int> renumberPinNo;
	int no=0;
	for(it=tmpItems.begin();it!=tmpItems.end();it++)
		if((*it)->type()==Wire::Type)
			if(GNDNode.find(((Wire *)(*it))->wireNo)!=GNDNode.end())
				((Wire *)(*it))->wireNo=0;
			else
				renumberPinNo[((Wire *)(*it))->wireNo]=-1;
	for(map<int,int>::iterator it=renumberPinNo.begin();it!=renumberPinNo.end();it++)
		(*it).second=++no;
	for(it=tmpItems.begin();it!=tmpItems.end();it++)
		if((*it)->type()==Wire::Type)
			if(GNDNode.find(((Wire *)(*it))->wireNo)==GNDNode.end())
				((Wire *)(*it))->wireNo=renumberPinNo[((Wire *)(*it))->wireNo];
	//将元件管脚编号设置为所连节点编号
	set<Element *> isConnectedToGND;
	for(it=tmpItems.begin();it!=tmpItems.end();it++)
		if((*it)->type()==PartView::Type)
			for(i=0;i<((PartView *)(*it))->part->pinNum;i++)
				if((((PartView *)(*it))->part->pin[i]=((PartView *)(*it))->partPin[i]->wireNo)==0)
					isConnectedToGND.insert(((PartView *)(*it))->part);//标记非孤立元件
	//检测是否有孤立元件，即每个连通分支是否均有至少一个地节点（不允许有悬浮的节点，即每个节点对地都必须有直流的通路）
	fList<Element *>::iterator eit;
	set<Element *> isConnected=isConnectedToGND;
	for(set<Element *>::iterator e_it=isConnectedToGND.begin();e_it!=isConnectedToGND.end();e_it++)
		findConnectedPart(*e_it,isConnected);
	for(eit=sceneOfSchematic->circuitParts.begin();eit!=sceneOfSchematic->circuitParts.end();++eit)
		if(isConnected.find(*eit)==isConnected.end())
		{
            QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("网表文件生成失败：存在孤立元件")+QString((*eit)->name.c_str())+QStringLiteral("等"));
			return false;
		}
	//写入网表
    ofstream outfile((fileName.split(".").first()+" - netlist.txt").toStdString(),ios::out);
	if(!outfile)
	{
		QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("网表文件保存失败：无法写入文件"));
		return false;
	}
	bool hasPart=false;
	for(eit=sceneOfSchematic->circuitParts.begin();eit!=sceneOfSchematic->circuitParts.end();++eit)
		if((*eit)->type!="GND")
		{
			outfile<<(*eit)->type<<'\t'<<**eit<<endl;
			hasPart=true;
		}
	outfile.close();
	if(!hasPart)
	{
		QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("网表文件保存失败：没有除GND外的元件"));
		return false;
	}
	statusBar()->showMessage(QStringLiteral("网表文件保存成功"),3000);
	return true;
}

bool MainWindow::saveSchematic()
{
	QString tmpFileName;
	if(fileName.isNull())
	{
		tmpFileName=QFileDialog::getSaveFileName(this,QStringLiteral("保存原理图文件"),"./",QStringLiteral("原理图文件(*.sch)"));
		if(tmpFileName.isNull())
			return false;
		else
			fileName=tmpFileName;
	}
    ofstream outfile(fileName.toStdString(),ios::out);
	if(!outfile)
	{
		QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("保存失败：无法写入文件"));
		return false;
	}
	else
	{
		isSaved=true;
		setWindowTitle(fileName.split("/").last()+QStringLiteral(" - 简易电路仿真器"));
	}
	outputItems(outfile,sceneOfSchematic->items());
	outfile.close();
	statusBar()->showMessage(QStringLiteral("保存成功"),3000);
	return true;
}

void MainWindow::outputItems(ostream &output,QList<QGraphicsItem *> items)
{//向output流输出items
	QList<QGraphicsItem *> tmpItems=items;
	QList<QGraphicsItem *>::iterator it;
	for(it=items.begin();it!=items.end();it++)
		if((*it)->type()==PartView::Type)
		{
			QList<QGraphicsItem *> tmpChildItems=(*it)->childItems();
			QPointF partPos=(*it)->pos();
			for(QList<QGraphicsItem *>::iterator _it=tmpChildItems.begin();_it!=tmpChildItems.end();_it++)
				if((*_it)->type()==QGraphicsPixmapItem::Type)
				{
					partPos=(*_it)->sceneBoundingRect().topLeft();
					switch(-(int)(*it)->rotation()%360/90)
					{
					case 1:partPos-=QPointF(0.0,1.0);break;
					case 2:partPos-=QPointF(1.0,1.0);break;
					case 3:partPos-=QPointF(1.0,0.0);break;
					default:break;
					}
					break;
				}
			output<<((PartView *)(*it))->part->type<<'\t'<<*((PartView *)(*it))->part
				<<(*it)->rotation()+((((PartView *)(*it))->part->type[0]=='V'||((PartView *)(*it))->part->type[0]=='I')?90.0:0.0)
				<<'\t'<<partPos.x()<<'\t'<<partPos.y()<<'\t';
			for(QList<Wire *>::iterator wit=((PartView *)(*it))->partPin.begin();wit!=((PartView *)(*it))->partPin.end();wit++)
				tmpItems.removeOne(*wit);
			for(QList<PartViewText *>::iterator pit=((PartView *)(*it))->partText.begin();pit!=((PartView *)(*it))->partText.end();pit++)
				output<<(*pit)->x()<<'\t'<<(*pit)->y()<<'\t';
			output<<endl;
		}
	for(it=tmpItems.begin();it!=tmpItems.end();it++)
		if((*it)->type()==Wire::Type)
		{
			QRectF tmpWire=((Wire *)(*it))->sceneBoundingRect().adjusted
				(WIRE_WIDTH_TO_SELECT/2.0+0.5,WIRE_WIDTH_TO_SELECT/2.0+0.5,-WIRE_WIDTH_TO_SELECT/2.0-0.5,-WIRE_WIDTH_TO_SELECT/2.0-0.5);
			output<<"WIRE"<<'\t'<<tmpWire.topLeft().x()<<'\t'<<tmpWire.topLeft().y()<<'\t'
				<<tmpWire.bottomRight().x()<<'\t'<<tmpWire.bottomRight().y()<<endl;
		}
	output<<"STATICN"<<'\t';
	Element::showStaticNo(output);
}

void MainWindow::newSchematic()
{
	if(isSaved||saveOnCloseQuestion())
	{
		delete sceneOfSchematic;
		sceneOfSchematic=newGraphicsScene();
		fileName.clear();
		sCurrentSelectPart.clear();
		currentSelectPart=NULL;
		isSaved=true;
		setWindowTitle(QStringLiteral("未命名 - 简易电路仿真器"));
		Ground::No=Resistor::No=Capacitor::No=Inductor::No=Diode::No
			=VoltageSource::No=CurrentSource::No=Transistor::No=0;
	}
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	if((event->key()==Qt::Key_Enter||event->key()==QKeySequence::InsertParagraphSeparator||event->key()==Qt::Key_Return)
		&&focusWidget()==ui->partList&&!ui->partList->selectedItems().isEmpty())
		initPart(ui->partList->selectedItems().first());
	if(event->key()==Qt::Key_Escape&&ui->viewOfSchematic->viewport()->cursor().shape()==Qt::CrossCursor)
	{
		ui->viewOfSchematic->viewport()->unsetCursor();
		sceneOfSchematic->isDrawingWire=false;
		sceneOfSchematic->mousePressCountWhenDrawingWire=0;
		//恢复QGraphicsItem属性，移除所有交点标识
		QList<QGraphicsItem *> tmpItems=sceneOfSchematic->items();
		for(QList<QGraphicsItem *>::iterator it=tmpItems.begin();it!=tmpItems.end();it++)
			if((*it)->type()==QGraphicsEllipseItem::Type)
			{
				sceneOfSchematic->removeItem((*it));
				delete *it;
			}
			else if((*it)->type()!=QGraphicsPixmapItem::Type)
			{
				(*it)->setAcceptDrops(true);
				(*it)->setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsFocusable);
			}
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
	if(event->key()==Qt::Key_G)
	{//创建地元件
		if(!ui->partList->findItems("GND",Qt::MatchContains).isEmpty())
			initPart(ui->partList->findItems("GND",Qt::MatchContains).first());
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if(!isSaved)
	{
		if(saveOnCloseQuestion())
			event->accept();
		else
			event->ignore();
	}
}

bool MainWindow::saveOnCloseQuestion()
{//询问是否保存。若保存或放弃，则返回true；若取消或打开失败，则返回false
	QMessageBox::StandardButton button=QMessageBox::question(this,QStringLiteral("简易电路仿真器"),
		QStringLiteral("文件")+fileName.split("/").last()+QStringLiteral("已被修改\n是否保存？"),
		QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);
	switch(button)
	{
	case QMessageBox::Save:
		if(!saveSchematic())//保存文件
		{
			if(!fileName.isNull())
				QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("无法保存文件"));
			return false;
		}
		else
			return true;
	case QMessageBox::Discard:return true;
	case QMessageBox::Cancel:return false;
	default:return true;
	}
}

void MainWindow::runSimulation()
{
	if(!isSaved&&!saveSchematic())//保存文件
	{
		QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("仿真失败：未保存原理图文件"));
		return;
	}
	if(saveNetlist())
	{
		Structure *structure=new Structure;
		if(!structure->initData(fileName.split(".").first()+" - netlist.txt"))
			QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("仿真失败：无法打开网表文件"));
		SimulationSetting simulationSetting(structure,this);
		if(simulationSetting.exec()==QDialog::Accepted)
		{
			switch(simulationSetting.ui->simulationType->currentIndex())
			{
			case 0:
				{//时域分析
					TimeDomainCalculate *calculate=new TimeDomainCalculate;
					double runTo,start,h;
					if(simulationSetting.ui->runTo->text()!="")
						runTo=etof(simulationSetting.ui->runTo->text().toStdString());
					else
						runTo=0.0;
					if(simulationSetting.ui->start->text()!="")
						start=abs(etof(simulationSetting.ui->start->text().toStdString()));
					else
						start=0.0;
					if(simulationSetting.ui->step->text()!="")
						h=abs(etof(simulationSetting.ui->step->text().toStdString()));
					else
						h=runTo/1024.0;
					calculate->timeDomain(h,runTo,structure,this);
					showSimulationText(h,calculate,structure);//绘制仿真结果标记
					statusBar()->showMessage(QStringLiteral("仿真成功"),3000);
					TimeDomainResult *timeDomainResult=new TimeDomainResult(runTo,start,h,calculate,structure);
					timeDomainResult->show();
					break;
				}
			case 1:
				{//直流扫描分析

					//主扫描设置
					int primarySweepType=Calculate::Linear;
					if(simulationSetting.ui->primaryLog->isChecked())
						primarySweepType=Calculate::Log;//获取主扫描类型

					QString primaryCheckedPartName;
					int primaryCheckedParameterNo;
					Element *primaryCheckedPart;
					for(int i=0;i<simulationSetting.ui->primaryPartList->count();i++)//获取主扫描元件名
						if(((QRadioButton *)simulationSetting.ui->primaryPartList->itemWidget(simulationSetting.ui->primaryPartList->item(i)))->isChecked())
						{
							primaryCheckedPartName=simulationSetting.ui->primaryPartList->item(i)->text().trimmed();
							break;
						}
					for(int i=0;i<simulationSetting.ui->primaryParameterList->count();i++)//获取主扫描元件参数名
						if(((QRadioButton *)simulationSetting.ui->primaryParameterList->itemWidget(simulationSetting.ui->primaryParameterList->item(i)))->isChecked())
						{
							primaryCheckedParameterNo=((QRadioButton *)simulationSetting.ui->primaryParameterList
								->itemWidget(simulationSetting.ui->primaryParameterList->item(i)))->objectName().toInt();
							break;
						}
					for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
                        if(QString((*it)->name.c_str())==primaryCheckedPartName)//获取主扫描元件指针
						{//假设无重名元件
							primaryCheckedPart=(*it);
							break;
						}

					double primaryStart;//获取主扫描起始、终止、步长等参数
					double primaryEnd;
					double primaryStep;
					if(simulationSetting.ui->primaryStart->text().toStdString()=="")
						primaryStart=primaryCheckedPart->showParameter(primaryCheckedParameterNo);//默认值
					else
						primaryStart=etof(simulationSetting.ui->primaryStart->text().toStdString());
					if(simulationSetting.ui->primaryEnd->text().toStdString()=="")
						primaryEnd=primaryCheckedPart->showParameter(primaryCheckedParameterNo);
					else
						primaryEnd=etof(simulationSetting.ui->primaryEnd->text().toStdString());
					if(simulationSetting.ui->primaryStep->text().toStdString()=="")
						primaryStep=1;
					else
						primaryStep=etof(simulationSetting.ui->primaryStep->text().toStdString());

					//副扫描设置（若无副扫描，则视为sweepNum=1的情形）
					DCSweepCalculate *calculate;
					int sweepNum=1;
					int secondarySweepType=Calculate::Unabled;
					int secondaryCheckedParameterNo=0;
					Element *secondaryCheckedPart=NULL;
					double secondaryStart=0.0;
					double secondaryEnd=0.0;
					double secondaryStep=0.0;

					if(simulationSetting.ui->enableSecondarySweep->isChecked())
						if(simulationSetting.ui->secondaryLog->isChecked())
							secondarySweepType=Calculate::Log;//获取副扫描类型
						else if(simulationSetting.ui->secondaryLinear->isChecked())
							secondarySweepType=Calculate::Linear;//获取副扫描类型
					
					if(secondarySweepType!=Calculate::Unabled)
					{
						QString secondaryCheckedPartName;
						for(int i=0;i<simulationSetting.ui->secondaryPartList->count();i++)//获取副扫描元件名
							if(((QRadioButton *)simulationSetting.ui->secondaryPartList->itemWidget(simulationSetting.ui->secondaryPartList->item(i)))->isChecked())
							{
								secondaryCheckedPartName=simulationSetting.ui->secondaryPartList->item(i)->text().trimmed();
								break;
							}
						for(int i=0;i<simulationSetting.ui->secondaryParameterList->count();i++)//获取副扫描元件参数名
							if(((QRadioButton *)simulationSetting.ui->secondaryParameterList->itemWidget(simulationSetting.ui->secondaryParameterList->item(i)))->isChecked())
							{
								secondaryCheckedParameterNo=((QRadioButton *)simulationSetting.ui->secondaryParameterList
									->itemWidget(simulationSetting.ui->secondaryParameterList->item(i)))->objectName().toInt();
								break;
							}
						for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
                            if(QString((*it)->name.c_str())==secondaryCheckedPartName)//获取副扫描元件指针
							{//假设无重名元件
								secondaryCheckedPart=(*it);
								break;
							}
						
						//获取副扫描起始、终止、步长等参数
						if(simulationSetting.ui->secondaryStart->text().toStdString()=="")
							secondaryStart=secondaryCheckedPart->showParameter(secondaryCheckedParameterNo);//默认值
						else
							secondaryStart=etof(simulationSetting.ui->secondaryStart->text().toStdString());
						if(simulationSetting.ui->secondaryEnd->text().toStdString()=="")
							secondaryEnd=secondaryCheckedPart->showParameter(secondaryCheckedParameterNo);
						else
							secondaryEnd=etof(simulationSetting.ui->secondaryEnd->text().toStdString());
						if(simulationSetting.ui->secondaryStep->text().toStdString()=="")
							secondaryStep=1;
						else
							secondaryStep=etof(simulationSetting.ui->secondaryStep->text().toStdString());
						if(secondarySweepType==Calculate::Linear)
						{
							sweepNum=abs((int)((secondaryEnd-secondaryStart)/secondaryStep));
							calculate=new DCSweepCalculate[sweepNum];
							for(int i=0;i<sweepNum;i++)
							{
								secondaryCheckedPart->setParameter(secondaryCheckedParameterNo,secondaryStart+sign(secondaryEnd-secondaryStart)*i*abs(secondaryStep));
								calculate[i].dCSweep(primaryCheckedPart,primaryCheckedParameterNo,primaryStart,primaryEnd,primaryStep,primarySweepType,structure,this);
							}
						}
						else if(secondarySweepType==Calculate::Log)
						{
							sweepNum=abs((int)log10(secondaryEnd/secondaryStart)*(int)secondaryStep);
							calculate=new DCSweepCalculate[sweepNum];
							for(int i=0;i<sweepNum;i++)
							{
								secondaryCheckedPart->setParameter(secondaryCheckedParameterNo,secondaryStart*pow(10.0,sign(secondaryEnd-secondaryStart)*abs((double)i/(int)secondaryStep)));
								calculate[i].dCSweep(primaryCheckedPart,primaryCheckedParameterNo,primaryStart,primaryEnd,primaryStep,primarySweepType,structure,this);
							}
						}
					}
					else
					{
						calculate=new DCSweepCalculate[sweepNum];
						calculate[0].dCSweep(primaryCheckedPart,primaryCheckedParameterNo,primaryStart,primaryEnd,primaryStep,primarySweepType,structure,this);
					}
					
					statusBar()->showMessage(QStringLiteral("仿真成功"),3000);//显示仿真结果
					DCSweepResult *dCSweepResult=new DCSweepResult(calculate,structure,primarySweepType,primaryStart,primaryEnd,primaryStep,primaryCheckedPart,primaryCheckedParameterNo,
						secondarySweepType,secondaryStart,secondaryEnd,secondaryStep,secondaryCheckedPart,secondaryCheckedParameterNo);
					dCSweepResult->show();
					break;
				}
			case 2:
				{//交流扫描分析

					//主扫描设置
					int ACSweepType=Calculate::Linear;
					if(simulationSetting.ui->ACLog->isChecked())
						ACSweepType=Calculate::Log;//获取主扫描类型

					fList<VAC *> ACPart;
					for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
						if((*it)->type=="VAC")//获取主扫描元件指针
							ACPart.push_front((VAC *)*it);

					double ACStart;//获取主扫描起始、终止、步长等参数
					double ACEnd;
					double ACStep;
					if(simulationSetting.ui->ACStart->text().toStdString()=="")
						ACStart=ACPart.front()->frequency;//默认值
					else
						ACStart=etof(simulationSetting.ui->ACStart->text().toStdString());
					if(simulationSetting.ui->ACEnd->text().toStdString()=="")
						ACEnd=ACPart.front()->frequency;//默认值
					else
						ACEnd=etof(simulationSetting.ui->ACEnd->text().toStdString());
					if(simulationSetting.ui->ACStep->text().toStdString()=="")
						ACStep=1;
					else
						ACStep=etof(simulationSetting.ui->ACStep->text().toStdString());

					//参数扫描设置（若无参数扫描，则视为sweepNum=1的情形）
					ACSweepCalculate *calculate;
					int sweepNum=1;
					int parametricSweepType=Calculate::Unabled;
					int parametricCheckedParameterNo=0;
					Element *parametricCheckedPart=NULL;
					double parametricStart=0.0;
					double parametricEnd=0.0;
					double parametricStep=0.0;

					if(simulationSetting.ui->enableParametricSweep->isChecked())
						if(simulationSetting.ui->parametricLog->isChecked())
							parametricSweepType=Calculate::Log;//获取参数扫描类型
						else if(simulationSetting.ui->parametricLinear->isChecked())
							parametricSweepType=Calculate::Linear;//获取参数扫描类型
					
					if(parametricSweepType!=Calculate::Unabled)
					{
						QString parametricCheckedPartName;
						for(int i=0;i<simulationSetting.ui->parametricPartList->count();i++)//获取参数扫描元件名
							if(((QRadioButton *)simulationSetting.ui->parametricPartList->itemWidget(simulationSetting.ui->parametricPartList->item(i)))->isChecked())
							{
								parametricCheckedPartName=simulationSetting.ui->parametricPartList->item(i)->text().trimmed();
								break;
							}
						for(int i=0;i<simulationSetting.ui->parametricParameterList->count();i++)//获取参数扫描元件参数名
							if(((QRadioButton *)simulationSetting.ui->parametricParameterList->itemWidget(simulationSetting.ui->parametricParameterList->item(i)))->isChecked())
							{
								parametricCheckedParameterNo=((QRadioButton *)simulationSetting.ui->parametricParameterList
									->itemWidget(simulationSetting.ui->parametricParameterList->item(i)))->objectName().toInt();
								break;
							}
						for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
                            if(QString((*it)->name.c_str())==parametricCheckedPartName)//获取参数扫描元件指针
							{//假设无重名元件
								parametricCheckedPart=(*it);
								break;
							}
						
						//获取参数扫描起始、终止、步长等参数
						if(simulationSetting.ui->parametricStart->text().toStdString()=="")
							parametricStart=parametricCheckedPart->showParameter(parametricCheckedParameterNo);//默认值
						else
							parametricStart=etof(simulationSetting.ui->parametricStart->text().toStdString());
						if(simulationSetting.ui->parametricEnd->text().toStdString()=="")
							parametricEnd=parametricCheckedPart->showParameter(parametricCheckedParameterNo);
						else
							parametricEnd=etof(simulationSetting.ui->parametricEnd->text().toStdString());
						if(simulationSetting.ui->parametricStep->text().toStdString()=="")
							parametricStep=1;
						else
							parametricStep=etof(simulationSetting.ui->parametricStep->text().toStdString());
						if(parametricSweepType==Calculate::Linear)
						{
							sweepNum=abs((int)((parametricEnd-parametricStart)/parametricStep));
							calculate=new ACSweepCalculate[sweepNum];
							for(int i=0;i<sweepNum;i++)
							{
								parametricCheckedPart->setParameter(parametricCheckedParameterNo,parametricStart+sign(parametricEnd-parametricStart)*i*abs(parametricStep));
								calculate[i].aCSweep(ACPart,ACStart,ACEnd,ACStep,ACSweepType,structure,this);
							}
						}
						else if(parametricSweepType==Calculate::Log)
						{
							sweepNum=abs((int)log10(parametricEnd/parametricStart)*(int)parametricStep);
							calculate=new ACSweepCalculate[sweepNum];
							for(int i=0;i<sweepNum;i++)
							{
								parametricCheckedPart->setParameter(parametricCheckedParameterNo,parametricStart*pow(10.0,sign(parametricEnd-parametricStart)*abs((double)i/(int)parametricStep)));
								calculate[i].aCSweep(ACPart,ACStart,ACEnd,ACStep,ACSweepType,structure,this);
							}
						}
					}
					else
					{
						calculate=new ACSweepCalculate[sweepNum];
						calculate[0].aCSweep(ACPart,ACStart,ACEnd,ACStep,ACSweepType,structure,this);
					}
					
					statusBar()->showMessage(QStringLiteral("仿真成功"),3000);//显示仿真结果
					ACSweepResult *aCSweepResult=new ACSweepResult(calculate,structure,ACSweepType,ACStart,ACEnd,ACStep,ACPart,
						parametricSweepType,parametricStart,parametricEnd,parametricStep,parametricCheckedPart,parametricCheckedParameterNo);
					aCSweepResult->show();
					break;
				}
			}
		}
	}
}

void MainWindow::showSimulationText(double h,Calculate *calculate,Structure *structure)
{
	QList<QGraphicsItem *> tmpItems=sceneOfSchematic->items();
	for(QList<QGraphicsItem *>::iterator it=tmpItems.begin();it!=tmpItems.end();it++)
		if((*it)->type()==PartView::Type&&((PartView *)(*it))->part->type!="GND")
		{
			//清除旧有仿真标记
			for(QList<PartViewText *>::iterator tit=((PartView *)(*it))->simulationText.begin();tit!=((PartView *)(*it))->simulationText.end();tit++)
			{
				((PartView *)(*it))->removeFromGroup(*tit);
				sceneOfSchematic->removeItem(*tit);
				delete *tit;
			}
			((PartView *)(*it))->simulationText.clear();
			//绘制新仿真标记
			Element *partInStructure=NULL;
			for(fList<Element *>::iterator eit=structure->parts.begin();eit!=structure->parts.end();++eit)
				if((*eit)->name==((PartView *)(*it))->part->name)
				{
					partInStructure=*eit;
					break;
				}
			for(int i=0;i<partInStructure->pinNum;i++)
			{
				PartViewText *biasVoltageText=new PartViewText(QString(ftoe(calculate->showResult(partInStructure,i,VOLTAGE,structure,0,h),4).c_str())+'V',(PartView *)(*it),-1,QColor(255,255,0,200));
				PartViewText *biasCurrentText=new PartViewText(QString(ftoe(calculate->showResult(partInStructure,i,CURRENT,structure,0,h),4).c_str())+'A',(PartView *)(*it),-1,QColor(0,255,255,200));
				biasVoltageText->setFont(QFont("Arial",9));
				biasCurrentText->setFont(QFont("Arial",9));
				QPointF textBasePos=((PartView *)(*it))->partPin[i]->sceneBoundingRect().adjusted(WIRE_WIDTH_TO_SELECT/2.0,WIRE_WIDTH_TO_SELECT/2.0,-WIRE_WIDTH_TO_SELECT/2.0,-WIRE_WIDTH_TO_SELECT/2.0).topLeft();
				if(((PartView *)(*it))->partPin[i]->sceneBoundingRect().width()>((PartView *)(*it))->partPin[i]->sceneBoundingRect().height())
				{
					if(((PartView *)(*it))->partPin[i]->boundingRect().x()<PARTVIEW_LENGTH/2.0)
					{
						biasVoltageText->setPos(textBasePos-QPointF(biasVoltageText->sceneBoundingRect().width(),biasVoltageText->sceneBoundingRect().height()));
						biasCurrentText->setPos(textBasePos-QPointF(biasVoltageText->sceneBoundingRect().width(),0.0));
					}
					else
					{
						biasVoltageText->setPos(textBasePos-QPointF(-PINVIEW_LENGTH,biasVoltageText->sceneBoundingRect().height()));
						biasCurrentText->setPos(textBasePos-QPointF(-PINVIEW_LENGTH,0.0));
					}
				}
				else
				{
					if(((PartView *)(*it))->partPin[i]->boundingRect().x()<PARTVIEW_LENGTH/2.0)
					{
						biasVoltageText->setPos(textBasePos-QPointF(biasVoltageText->sceneBoundingRect().width(),-PINVIEW_LENGTH));
						biasCurrentText->setPos(textBasePos-QPointF(0.0,-PINVIEW_LENGTH));
					}
					else
					{
						biasVoltageText->setPos(textBasePos-QPointF(biasVoltageText->sceneBoundingRect().width(),biasVoltageText->sceneBoundingRect().height()));
						biasCurrentText->setPos(textBasePos-QPointF(0.0,biasVoltageText->sceneBoundingRect().height()));
					}
				}
				((PartView *)(*it))->simulationText.push_back(biasVoltageText);
				((PartView *)(*it))->simulationText.push_back(biasCurrentText);
				((PartView *)(*it))->addToGroup(biasVoltageText);
				((PartView *)(*it))->addToGroup(biasCurrentText);
			}
		}
}

void MainWindow::findConnectedPart(Element *currentPart,set<Element *> &isConnected)
{
	for(int i=0;i<currentPart->pinNum;i++)
		for(fList<Element *>::iterator it=sceneOfSchematic->circuitParts.begin();it!=sceneOfSchematic->circuitParts.end();++it)
			for(int j=0;j<(*it)->pinNum;j++)
				if(isConnected.find(*it)==isConnected.end()&&(*it)->pin[j]==currentPart->pin[i])
				{
					isConnected.insert(*it);
					findConnectedPart(*it,isConnected);
				}
}

bool MainWindow::copy()
{
	if(ui->viewOfSchematic->hasFocus()&&!sceneOfSchematic->selectedItems().isEmpty())
	{
		ostringstream copyPartsInfo;
		outputItems(copyPartsInfo,sceneOfSchematic->selectedItems());
		QMimeData *mimeData=new QMimeData;
		mimeData->setData("application/element",QByteArray(copyPartsInfo.str().c_str(),copyPartsInfo.str().size()+1));
		QApplication::clipboard()->clear();
		QApplication::clipboard()->setMimeData(mimeData);
		statusBar()->showMessage(QStringLiteral("已复制"),3000);
		return true;
	}
	return false;
}

void MainWindow::paste()
{
	if(ui->viewOfSchematic->hasFocus()&&QApplication::clipboard()->mimeData()->hasFormat("application/element"))
	{
		istringstream pastePartsInfo(string(QApplication::clipboard()->mimeData()->data("application/element").data()));
		QList<QGraphicsItem *> tmpParts=inputItems(pastePartsInfo,true);
		if(!tmpParts.isEmpty())
		{
			sceneOfSchematic->currentPlacingParts=tmpParts;
			sceneOfSchematic->currentPlacingPartGroup=sceneOfSchematic->createItemGroup(sceneOfSchematic->currentPlacingParts);
			sceneOfSchematic->isPlacingPart=true;
        }
        isSaved=false;//设置保存状态
        setWindowTitle(windowTitle().split(QRegExp("[* ]")).first()+QStringLiteral("* - 简易电路仿真器"));
	}
}

void MainWindow::cut()
{
	if(copy())
	{
		QKeyEvent *deleteKey=new QKeyEvent(QEvent::KeyPress,Qt::Key_Backspace,Qt::NoModifier);
		sceneOfSchematic->keyPressEvent(deleteKey);
		delete deleteKey;
	}
}

void MainWindow::selectAll()
{
	QList<QGraphicsItem *> tmpItems=sceneOfSchematic->items();
	for(QList<QGraphicsItem *>::iterator it=tmpItems.begin();it!=tmpItems.end();it++)
		(*it)->setSelected(true);
}

void MainWindow::deleteItem()
{
	if(ui->viewOfSchematic->hasFocus()&&!sceneOfSchematic->selectedItems().isEmpty())
	{
		QKeyEvent *deleteKey=new QKeyEvent(QEvent::KeyPress,Qt::Key_Backspace,Qt::NoModifier);
		sceneOfSchematic->keyPressEvent(deleteKey);
		delete deleteKey;
	}
}

void MainWindow::help()
{
	QDialog *help=new QDialog(this);
	help->setWindowFlags(Qt::Window);
	help->setWindowFlags(windowFlags()&~Qt::WindowStaysOnTopHint);
	help->setAttribute(Qt::WA_DeleteOnClose);
	help->setWindowTitle(QStringLiteral("帮助"));
	help->resize(1000,800);
	QTextBrowser *textBrowser = new QTextBrowser(this);
	QGridLayout *gridLayout=new QGridLayout(this);
	gridLayout->addWidget(textBrowser);
	help->setLayout(gridLayout);
	textBrowser->setSearchPaths(QStringList()<<":/help");
	textBrowser->setSource(QString("help"));
	help->show();
}
