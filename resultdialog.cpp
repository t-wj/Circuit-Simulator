#include "resultdialog.h"
#include "ui_timedomainresult.h"
#include "ui_dcsweepresult.h"
#include "ui_acsweepresult.h"
#include "mainwindow.h"

TimeDomainResult::TimeDomainResult(double _runTo,double _start,double _h,TimeDomainCalculate *_calculate,Structure *_structure,QWidget *parent) :
	runTo(_runTo),start(_start),h(_h),calculate(_calculate),structure(_structure),QDialog(parent),
    ui(new Ui::TimeDomainResult)
{
	ui->setupUi(this);
	setWindowFlags(Qt::Window);
	setWindowFlags(windowFlags()&~Qt::WindowStaysOnTopHint);
	setAttribute(Qt::WA_DeleteOnClose);

	//设置变量列表
	QListWidgetItem *item=NULL;
	for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
		for(int i=0;i<(*it)->pinNum;i++)
		{
            item=new QListWidgetItem("V("+QString((*it)->name.c_str())+':'+QString::number(i)+')',ui->variableList);
			item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);
			item->setCheckState(Qt::Unchecked);
            item=new QListWidgetItem("I("+QString((*it)->name.c_str())+':'+QString::number(i)+')',ui->variableList);
			item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);
			item->setCheckState(Qt::Unchecked);
		}
	ui->variableList->sortItems();
	
	//设置背景色、坐标轴颜色与字体等
    ui->resultPlot->xAxis->setLabelColor(Qt::white);
    ui->resultPlot->xAxis->setBasePen(QPen(Qt::white));
	ui->resultPlot->xAxis->setSubTickPen(QPen(Qt::white));
	ui->resultPlot->xAxis->setTickPen(QPen(Qt::white));
	ui->resultPlot->xAxis->setTickLabelColor(Qt::white);
	ui->resultPlot->xAxis->setLabelFont(QFont("Courier New",12));
	ui->resultPlot->xAxis->setNumberFormat("g");
	ui->resultPlot->xAxis2->setNumberFormat("g");
	ui->resultPlot->yAxis->setNumberFormat("g");
	ui->resultPlot->yAxis2->setNumberFormat("g");
	QList<QCPAxis *> axes;
	axes.push_back(ui->resultPlot->xAxis);
	axes.push_back(ui->resultPlot->xAxis2);
	axes.push_back(ui->resultPlot->yAxis);
	axes.push_back(ui->resultPlot->yAxis2);
	ui->resultPlot->axisRect()->setupFullAxesBox();
	ui->resultPlot->axisRect()->setRangeDragAxes(axes);
	ui->resultPlot->axisRect()->setRangeZoomAxes(axes);
	ui->resultPlot->setBackground(Qt::black);
	ui->resultPlot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom|QCP::iSelectPlottables);
}

void TimeDomainResult::plotCheckedVariables()
{
	//初始化为空视图
	for(int i=0;i<ui->resultPlot->plottableCount();i++)
		((QCPCurve *)ui->resultPlot->plottable(i))->data()->clear();
	ui->resultPlot->xAxis->setVisible(false);
	ui->resultPlot->xAxis2->setVisible(false);
	ui->resultPlot->yAxis->setVisible(false);
	ui->resultPlot->yAxis2->setVisible(false);
	ui->resultPlot->xAxis->setScaleType(QCPAxis::stLinear);
	ui->resultPlot->xAxis2->setScaleType(QCPAxis::stLinear);
	ui->resultPlot->yAxis->setScaleType(QCPAxis::stLinear);
	ui->resultPlot->yAxis2->setScaleType(QCPAxis::stLinear);
	ui->resultPlot->legend->setVisible(false);
	ui->resultPlot->legend->clear();

	//设置横坐标
	ui->resultPlot->xAxis->setVisible(true);
	ui->resultPlot->xAxis->setTickLabels(true);
	QVector<double> x;
	QString xAxisLabel=ui->xAxixVariable->text();
	if(ui->isFFT->checkState()==Qt::Checked)
	{//横轴为频率
		for(int i=0;i<(runTo-start)/h/2;i++)
			x.push_back((int)((double)i/(runTo-start)));
		xAxisLabel+="(Hz)";
	}
	else if(ui->xAxixVariable->text()=="time"||ui->xAxixVariable->text()=="Time")
	{//横轴为时间
		for(int i=(int)(start/h);i<=(int)(runTo/h);i++)
			x.push_back(i*h);
		xAxisLabel+="(s)";
	}
	else
	{//横轴为变量
		QStringList xsections=ui->xAxixVariable->text().split(QRegExp("[\(:\)]"));
		if(xsections.size()!=4)
			return;
		Element *xpart=NULL;
		for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
            if(QString((*it)->name.c_str())==xsections[1])
			{//假设无重名元件
				xpart=(*it);
				break;
			}
		if(xpart==NULL)
			return;
		if(xsections.first()=="V")
		{
			for(int i=(int)(start/h);i<=(int)(runTo/h);i++)
				x.push_back(calculate->showResult(xpart,xsections[2].toInt(),VOLTAGE,structure,i,h));
			xAxisLabel+="(V)";
		}
		else if(xsections.first()=="I")
		{
			for(int i=(int)(start/h);i<=(int)(runTo/h);i++)
				x.push_back(calculate->showResult(xpart,xsections[2].toInt(),CURRENT,structure,i,h));
			xAxisLabel+="(A)";
		}
		else
			return;
	}
	ui->resultPlot->xAxis->setLabel(xAxisLabel);

	//设置是否对数坐标
	if(ui->xAxisLog->checkState()==Qt::Checked)
	{
		ui->resultPlot->xAxis->setScaleType(QCPAxis::stLogarithmic);
		ui->resultPlot->xAxis2->setScaleType(QCPAxis::stLogarithmic);
	}
	if(ui->yAxisLog->checkState()==Qt::Checked)
		ui->resultPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
	if(ui->yAxis2Log->checkState()==Qt::Checked)
		ui->resultPlot->yAxis2->setScaleType(QCPAxis::stLogarithmic);

	//绘制图像
	int no=0;
	for(int i=0;i<ui->variableList->count();i++)
		if(ui->variableList->item(i)->checkState()==Qt::Checked)
			plotCurve(x,ui->variableList->item(i)->text(),no++);
	if(ui->resultPlot->legend->rowCount()>0)
	{
		ui->resultPlot->legend->setVisible(true);
		ui->resultPlot->legend->setFont(QFont("Courier New",10));
		ui->resultPlot->legend->setTextColor(Qt::white);
		ui->resultPlot->legend->setBrush(Qt::black);
		ui->resultPlot->legend->setBorderPen(QPen(Qt::white,1));
		ui->resultPlot->yAxis->grid()->setPen(QPen(QColor(0,0,0,0)));
	}
	ui->resultPlot->rescaleAxes();
	ui->resultPlot->replot();
}

void TimeDomainResult::plotCurve(QVector<double> &x,QString curveName,int curveOnScreenNo)
{
	QStringList sections=curveName.split(QRegExp("[\(:\)]"));
	Element *part=NULL;
	for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
        if(QString((*it)->name.c_str())==sections[1])
		{//假设无重名元件
			part=(*it);
			break;
		}
	if(part==NULL)
		return;
	if(sections.first()=="V")
	{
		QCPCurve *curve=new QCPCurve(ui->resultPlot->xAxis, ui->resultPlot->yAxis);

		QVector<QCPCurveData> data;//设置数据
		if(ui->isFFT->checkState()==Qt::Checked)
		{
			Matrix tmp((runTo-start)/h,1);
			for(int i=0;i<(int)((runTo-start)/h);i++)
				tmp(i,0)=calculate->showResult(part,sections[2].toInt(),VOLTAGE,structure,i+(int)(start/h),h);
			Matrix fftResult=tmp.FFT();
			const int point=(runTo-start)/h/2;
			for(int i=0;i<point;i++)
				data.push_back(QCPCurveData(i,x[i],sqrt(pow(fftResult(i,0),2)+pow(fftResult(i,1),2))/point));
		}
		else
		{
			for(int i=(int)(start/h);i<=(int)(runTo/h);i++)
				data.push_back(QCPCurveData(i,x[i-(int)(start/h)],calculate->showResult(part,sections[2].toInt(),VOLTAGE,structure,i,h)));
		}
		curve->data()->set(data,true);//添加数据
        curve->setPen(QPen(Qt::GlobalColor(Qt::red+(curveOnScreenNo%(Qt::yellow-Qt::red))),2));//设置曲线颜色
		ui->resultPlot->yAxis->setVisible(true);
		ui->resultPlot->yAxis->setTickLabels(true);
		ui->resultPlot->yAxis->setLabelColor(Qt::white);
		ui->resultPlot->yAxis->setBasePen(QPen(Qt::white));
		ui->resultPlot->yAxis->setLabelFont(QFont("Courier New",12));
		ui->resultPlot->yAxis->setLabel("Voltage(V)");
		ui->resultPlot->yAxis->setSubTickPen(QPen(Qt::white));
		ui->resultPlot->yAxis->setTickPen(QPen(Qt::white));
		ui->resultPlot->yAxis->setTickLabelColor(Qt::white);
		curve->setName(curveName);//设置曲线名
	}
	else if(sections.first()=="I")
	{
		QCPCurve *curve=new QCPCurve(ui->resultPlot->xAxis, ui->resultPlot->yAxis2);

		QVector<QCPCurveData> data;//设置数据
		if(ui->isFFT->checkState()==Qt::Checked)
		{
			Matrix tmp((runTo-start)/h,1);
			for(int i=0;i<(int)((runTo-start)/h);i++)
				tmp(i,0)=calculate->showResult(part,sections[2].toInt(),CURRENT,structure,i+(int)(start/h),h);
			Matrix fftResult=tmp.FFT();
			const int point=(runTo-start)/h/2;
			for(int i=0;i<point;i++)
				data.push_back(QCPCurveData(i,x[i],sqrt(pow(fftResult(i,0),2)+pow(fftResult(i,1),2))/point));
		}
		else
		{
			for(int i=(int)(start/h);i<=(int)(runTo/h);i++)
				data.push_back(QCPCurveData(i,x[i-(int)(start/h)],calculate->showResult(part,sections[2].toInt(),CURRENT,structure,i,h)));
		}
		curve->data()->set(data,true);//添加数据
        curve->setPen(QPen(Qt::GlobalColor(Qt::red+(curveOnScreenNo%(Qt::yellow-Qt::red))),2));//设置曲线颜色
		ui->resultPlot->yAxis2->setVisible(true);
		ui->resultPlot->yAxis2->setTickLabels(true);
		ui->resultPlot->yAxis2->setLabelColor(Qt::white);
		ui->resultPlot->yAxis2->setBasePen(QPen(Qt::white));
		ui->resultPlot->yAxis2->setLabelFont(QFont("Courier New",12));
		ui->resultPlot->yAxis2->setLabel("Current(A)");
		ui->resultPlot->yAxis2->setSubTickPen(QPen(Qt::white));
		ui->resultPlot->yAxis2->setTickPen(QPen(Qt::white));
		ui->resultPlot->yAxis2->setTickLabelColor(Qt::white);
		curve->setName(curveName);//设置曲线名
	}
}

void TimeDomainResult::fftSetting()
{
	if(ui->isFFT->checkState()==Qt::Checked)
	{
		ui->xAxixVariable->setText("frequency");
		ui->xAxixVariable->setEnabled(false);
	}
	else
	{
		ui->xAxixVariable->setText("time");
		ui->xAxixVariable->setEnabled(true);
	}
	plotCheckedVariables();
}

void TimeDomainResult::setXAxisVariable(QListWidgetItem *item)
{
	ui->xAxixVariable->setText(item->text());
	plotCheckedVariables();
}

void TimeDomainResult::searchVariable(QString variable)
{
	QList<QListWidgetItem *> searchItems=ui->variableList->findItems(variable,Qt::MatchContains);
	for(int i=0;i<ui->variableList->count();i++)
		ui->variableList->setRowHidden(i,true);//隐藏所有行
	for(int i=0;i<searchItems.count();i++)
		ui->variableList->setItemHidden(searchItems.at(i),false);//显示查找到的信息
}

TimeDomainResult::~TimeDomainResult()
{
	delete calculate;
	delete structure;
    delete ui;
}

DCSweepResult::DCSweepResult(DCSweepCalculate *_calculate,Structure *_structure,
							 int _primarySweepType,double _primaryStart,double _primaryEnd,double _primaryStep,Element *_primaryPart,int _primaryParameterNo,
							 int _secondarySweepType,double _secondaryStart,double _secondaryEnd,double _secondaryStep,
							 Element *_secondaryPart,int _secondaryParameterNo,QWidget *parent):
	primaryStart(_primaryStart),primaryEnd(_primaryEnd),primaryStep(_primaryStep),primarySweepType(_primarySweepType),primaryPart(_primaryPart),primaryParameterNo(_primaryParameterNo),
	secondaryStart(_secondaryStart),secondaryEnd(_secondaryEnd),secondaryStep(_secondaryStep),secondarySweepType(_secondarySweepType),
	secondaryPart(_secondaryPart),secondaryParameterNo(_secondaryParameterNo),calculate(_calculate),structure(_structure),QDialog(parent),
	ui(new Ui::DCSweepResult)
{
	ui->setupUi(this);
	setWindowFlags(Qt::Window);
	setWindowFlags(windowFlags()&~Qt::WindowStaysOnTopHint);
	setAttribute(Qt::WA_DeleteOnClose);

	//设置变量列表
	QListWidgetItem *item=NULL;
	for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
		for(int i=0;i<(*it)->pinNum;i++)
		{
            item=new QListWidgetItem("V("+QString((*it)->name.c_str())+':'+QString::number(i)+')',ui->variableList);
			item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);
			item->setCheckState(Qt::Unchecked);
            item=new QListWidgetItem("I("+QString((*it)->name.c_str())+':'+QString::number(i)+')',ui->variableList);
			item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);
			item->setCheckState(Qt::Unchecked);
		}
	ui->variableList->sortItems();
	
	//设置背景色、坐标轴颜色与字体等
    ui->resultPlot->xAxis->setLabelColor(Qt::white);
    ui->resultPlot->xAxis->setBasePen(QPen(Qt::white));
	ui->resultPlot->xAxis->setSubTickPen(QPen(Qt::white));
	ui->resultPlot->xAxis->setTickPen(QPen(Qt::white));
	ui->resultPlot->xAxis->setTickLabelColor(Qt::white);
	ui->resultPlot->xAxis->setLabelFont(QFont("Courier New",12));
	ui->resultPlot->xAxis->setNumberFormat("g");
	ui->resultPlot->xAxis2->setNumberFormat("g");
	ui->resultPlot->yAxis->setNumberFormat("g");
	ui->resultPlot->yAxis2->setNumberFormat("g");
	QList<QCPAxis *> axes;
	axes.push_back(ui->resultPlot->xAxis);
	axes.push_back(ui->resultPlot->xAxis2);
	axes.push_back(ui->resultPlot->yAxis);
	axes.push_back(ui->resultPlot->yAxis2);
	ui->resultPlot->axisRect()->setupFullAxesBox();
	ui->resultPlot->axisRect()->setRangeDragAxes(axes);
	ui->resultPlot->axisRect()->setRangeZoomAxes(axes);
	ui->resultPlot->setBackground(Qt::black);
	ui->resultPlot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom|QCP::iSelectPlottables);

	//设置横坐标
	ui->resultPlot->xAxis->setVisible(true);
	ui->resultPlot->xAxis->setTickLabels(true);
    ui->resultPlot->xAxis->setLabel(QString(primaryPart->name.c_str())+"--"+QString(primaryPart->showParameterViewName(primaryParameterNo)).replace(QRegExp("[\\x4e00-\\x9fa5]+"),""));
	if(primarySweepType==Calculate::Linear)
	{
		primarySweepNum=abs((int)((primaryStart-primaryEnd)/primaryStep));
		for(int i=0;i<primarySweepNum;i++)
			x.push_back(primaryStart+sign(primaryEnd-primaryStart)*i*abs(primaryStep));
		ui->resultPlot->xAxis->setScaleType(QCPAxis::stLinear);
	}
	else if(primarySweepType==Calculate::Log)
	{
		primarySweepNum=abs((int)log10(primaryStart/primaryEnd)*(int)primaryStep);
		for(int i=0;i<primarySweepNum;i++)
			x.push_back(primaryStart*pow(10.0,sign(primaryEnd-primaryStart)*abs((double)i/(int)primaryStep)));
		ui->resultPlot->xAxis->setScaleType(QCPAxis::stLogarithmic);
	}
	if(secondarySweepType==Calculate::Linear)
		secondarySweepNum=abs((int)((secondaryStart-secondaryEnd)/secondaryStep));
	else if(secondarySweepType==Calculate::Log)
		secondarySweepNum=abs((int)log10(secondaryStart/secondaryEnd)*(int)secondaryStep);
	else
		secondarySweepNum=1;
}

void DCSweepResult::plotCheckedVariables()
{
	//初始化为空视图
	for(int i=0;i<ui->resultPlot->plottableCount();i++)
		((QCPCurve *)ui->resultPlot->plottable(i))->data()->clear();
	ui->resultPlot->yAxis->setVisible(false);
	ui->resultPlot->yAxis2->setVisible(false);
	ui->resultPlot->yAxis->setScaleType(QCPAxis::stLinear);
	ui->resultPlot->yAxis2->setScaleType(QCPAxis::stLinear);
	ui->resultPlot->legend->setVisible(false);
	ui->resultPlot->legend->clear();

	//设置是否对数坐标
	if(ui->yAxisLog->checkState()==Qt::Checked)
		ui->resultPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
	if(ui->yAxis2Log->checkState()==Qt::Checked)
		ui->resultPlot->yAxis2->setScaleType(QCPAxis::stLogarithmic);

	//绘制图像
	int no=0;
	for(int i=0;i<ui->variableList->count();i++)
		if(ui->variableList->item(i)->checkState()==Qt::Checked)
			if(secondarySweepType==Calculate::Unabled)
				plotCurve(ui->variableList->item(i)->text(),no++,0);
			else if(secondarySweepType==Calculate::Linear)
			{
				for(int j=0;j<secondarySweepNum;j++)
				{
					secondaryPart->setParameter(secondaryParameterNo,secondaryStart+sign(secondaryEnd-secondaryStart)*j*abs(secondaryStep));
					plotCurve(ui->variableList->item(i)->text(),no,j);
				}
				no++;
			}
			else if(secondarySweepType==Calculate::Log)
			{
				for(int j=0;j<secondarySweepNum;j++)
				{
					secondaryPart->setParameter(secondaryParameterNo,secondaryStart*pow(10.0,sign(secondaryEnd-secondaryStart)*abs((double)j/(int)secondaryStep)));
					plotCurve(ui->variableList->item(i)->text(),no,j);
				}
				no++;
			}
	if(ui->resultPlot->legend->rowCount()>0)
	{
		ui->resultPlot->legend->setVisible(true);
		ui->resultPlot->legend->setFont(QFont("Courier New",10));
		ui->resultPlot->legend->setTextColor(Qt::white);
		ui->resultPlot->legend->setBrush(Qt::black);
		ui->resultPlot->legend->setBorderPen(QPen(Qt::white,1));
		ui->resultPlot->yAxis->grid()->setPen(QPen(QColor(0,0,0,0)));
	}
	ui->resultPlot->rescaleAxes();
	ui->resultPlot->replot();
}

void DCSweepResult::plotCurve(QString curveName,int curveOnScreenNo,int secondaryIndex)
{
	QStringList sections=curveName.split(QRegExp("[\(:\)]"));
	Element *part=NULL;
	for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
        if(QString((*it)->name.c_str())==sections[1])
		{//假设无重名元件
			part=(*it);
			break;
		}
	if(part==NULL)
		return;
	if(sections.first()=="V")
	{
		QCPCurve *curve=new QCPCurve(ui->resultPlot->xAxis, ui->resultPlot->yAxis);

		QVector<QCPCurveData> data;//设置数据
		for(int i=0;i<primarySweepNum;i++)
		{
			QCoreApplication::processEvents();//计算过程中保持程序响应
			if(primarySweepType==Calculate::Linear)
			{
				primaryPart->setParameter(primaryParameterNo,primaryStart+sign(primaryEnd-primaryStart)*i*abs(primaryStep));
				data.push_back(QCPCurveData(i,x[i],calculate[secondaryIndex].showResult(part,sections[2].toInt(),VOLTAGE,structure,i)));
			}
			else if(primarySweepType==Calculate::Log)
			{
				primaryPart->setParameter(primaryParameterNo,primaryStart*pow(10.0,sign(primaryEnd-primaryStart)*abs((double)i/(int)primaryStep)));
				data.push_back(QCPCurveData(i,x[i],calculate[secondaryIndex].showResult(part,sections[2].toInt(),VOLTAGE,structure,i)));
			}
		}
		curve->data()->set(data,true);//添加数据
		QColor curveColor=Qt::GlobalColor(Qt::red+(curveOnScreenNo%(Qt::yellow-Qt::red)));//设置曲线颜色
		int h=0,s=0,v=0;
		curveColor.getHsv(&h,&s,&v);
		s-=(int)(128.0*(double)secondaryIndex/(double)secondarySweepNum);
		curveColor.setHsv(h,s,v);
        curve->setPen(QPen(curveColor,2));
		ui->resultPlot->yAxis->setVisible(true);
		ui->resultPlot->yAxis->setTickLabels(true);
		ui->resultPlot->yAxis->setLabelColor(Qt::white);
		ui->resultPlot->yAxis->setBasePen(QPen(Qt::white));
		ui->resultPlot->yAxis->setLabelFont(QFont("Courier New",12));
		ui->resultPlot->yAxis->setLabel("Voltage(V)");
		ui->resultPlot->yAxis->setSubTickPen(QPen(Qt::white));
		ui->resultPlot->yAxis->setTickPen(QPen(Qt::white));
		ui->resultPlot->yAxis->setTickLabelColor(Qt::white);
		curve->setName(curveName);//设置曲线名
		if(secondaryIndex!=0)
			curve->removeFromLegend();
	}
	else if(sections.first()=="I")
	{
		QCPCurve *curve=new QCPCurve(ui->resultPlot->xAxis, ui->resultPlot->yAxis2);

		QVector<QCPCurveData> data;//设置数据
		for(int i=0;i<primarySweepNum;i++)
		{
			QCoreApplication::processEvents();//计算过程中保持程序响应
			if(primarySweepType==Calculate::Linear)
			{
				primaryPart->setParameter(primaryParameterNo,primaryStart+sign(primaryEnd-primaryStart)*i*abs(primaryStep));
				data.push_back(QCPCurveData(i,x[i],calculate[secondaryIndex].showResult(part,sections[2].toInt(),CURRENT,structure,i)));
			}
			else if(primarySweepType==Calculate::Log)
			{
				primaryPart->setParameter(primaryParameterNo,primaryStart*pow(10.0,sign(primaryEnd-primaryStart)*abs((double)i/(int)primaryStep)));
				data.push_back(QCPCurveData(i,x[i],calculate[secondaryIndex].showResult(part,sections[2].toInt(),CURRENT,structure,i)));
			}
		}
		curve->data()->set(data,true);//添加数据
		QColor curveColor=Qt::GlobalColor(Qt::red+(curveOnScreenNo%(Qt::yellow-Qt::red)));//设置曲线颜色
		int h=0,s=0,v=0;//设置曲线颜色
		curveColor.getHsv(&h,&s,&v);
		s-=(int)(200.0*(double)secondaryIndex/(double)secondarySweepNum);
		curveColor.setHsv(h,s,v);
        curve->setPen(QPen(curveColor,2));
		ui->resultPlot->yAxis2->setVisible(true);
		ui->resultPlot->yAxis2->setTickLabels(true);
		ui->resultPlot->yAxis2->setLabelColor(Qt::white);
		ui->resultPlot->yAxis2->setBasePen(QPen(Qt::white));
		ui->resultPlot->yAxis2->setLabelFont(QFont("Courier New",12));
		ui->resultPlot->yAxis2->setLabel("Current(A)");
		ui->resultPlot->yAxis2->setSubTickPen(QPen(Qt::white));
		ui->resultPlot->yAxis2->setTickPen(QPen(Qt::white));
		ui->resultPlot->yAxis2->setTickLabelColor(Qt::white);
		curve->setName(curveName);//设置曲线名
		if(secondaryIndex!=0)
			curve->removeFromLegend();
	}
}

void DCSweepResult::searchVariable(QString variable)
{
	QList<QListWidgetItem *> searchItems=ui->variableList->findItems(variable,Qt::MatchContains);
	for(int i=0;i<ui->variableList->count();i++)
		ui->variableList->setRowHidden(i,true);//隐藏所有行
	for(int i=0;i<searchItems.count();i++)
		ui->variableList->setItemHidden(searchItems.at(i),false);//显示查找到的信息
}

DCSweepResult::~DCSweepResult()
{
	delete []calculate;
	delete structure;
    delete ui;
}

ACSweepResult::ACSweepResult(ACSweepCalculate *_calculate,Structure *_structure,
							 int _ACSweepType,double _ACStart,double _ACEnd,double _ACStep,fList<VAC *> _ACParts,
							 int _parametricSweepType,double _parametricStart,double _parametricEnd,double _parametricStep,
							 Element *_parametricPart,int _parametricParameterNo,QWidget *parent):
	ACStart(_ACStart),ACEnd(_ACEnd),ACStep(_ACStep),ACSweepType(_ACSweepType),ACParts(_ACParts),
	parametricStart(_parametricStart),parametricEnd(_parametricEnd),parametricStep(_parametricStep),parametricSweepType(_parametricSweepType),
	parametricPart(_parametricPart),parametricParameterNo(_parametricParameterNo),calculate(_calculate),structure(_structure),QDialog(parent),
    ui(new Ui::ACSweepResult)
{
    ui->setupUi(this);
	setWindowFlags(Qt::Window);
	setWindowFlags(windowFlags()&~Qt::WindowStaysOnTopHint);
	setAttribute(Qt::WA_DeleteOnClose);

	//设置变量列表
	QListWidgetItem *item=NULL;
	for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
		for(int i=0;i<(*it)->pinNum;i++)
		{
            item=new QListWidgetItem("V("+QString((*it)->name.c_str())+':'+QString::number(i)+')',ui->variableList);
			item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);
			item->setCheckState(Qt::Unchecked);
			item=new QListWidgetItem("Phase("+item->text()+")",ui->variableList);
			item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);
			item->setCheckState(Qt::Unchecked);
            item=new QListWidgetItem("I("+QString((*it)->name.c_str())+':'+QString::number(i)+')',ui->variableList);
			item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);
			item->setCheckState(Qt::Unchecked);
			item=new QListWidgetItem("Phase("+item->text()+")",ui->variableList);
			item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);
			item->setCheckState(Qt::Unchecked);
		}
	ui->variableList->sortItems();

	//设置背景色、坐标轴颜色与字体等
    ui->resultPlot->xAxis->setLabelColor(Qt::white);
    ui->resultPlot->xAxis->setBasePen(QPen(Qt::white));
	ui->resultPlot->xAxis->setSubTickPen(QPen(Qt::white));
	ui->resultPlot->xAxis->setTickPen(QPen(Qt::white));
	ui->resultPlot->xAxis->setTickLabelColor(Qt::white);
	ui->resultPlot->xAxis->setLabelFont(QFont("Courier New",12));
	ui->resultPlot->xAxis->setNumberFormat("g");
	ui->resultPlot->xAxis2->setNumberFormat("g");
	ui->resultPlot->yAxis->setNumberFormat("g");
	ui->resultPlot->yAxis2->setNumberFormat("g");
	QList<QCPAxis *> axes;
	axes.push_back(ui->resultPlot->xAxis);
	axes.push_back(ui->resultPlot->xAxis2);
	axes.push_back(ui->resultPlot->yAxis);
	axes.push_back(ui->resultPlot->yAxis2);
	axes.push_back(ui->resultPlot->axisRect()->addAxis(QCPAxis::atLeft));
	ui->resultPlot->axisRect()->setupFullAxesBox();
	ui->resultPlot->axisRect()->setRangeDragAxes(axes);
	ui->resultPlot->axisRect()->setRangeZoomAxes(axes);
	ui->resultPlot->setBackground(Qt::black);
	ui->resultPlot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom|QCP::iSelectPlottables);

	//设置横坐标
	ui->resultPlot->xAxis->setVisible(true);
	ui->resultPlot->xAxis->setTickLabels(true);
	ui->resultPlot->xAxis->setLabel("frequency(Hz)");
	if(ACSweepType==Calculate::Linear)
	{
		ACSweepNum=abs((int)((ACStart-ACEnd)/ACStep));
		for(int i=0;i<ACSweepNum;i++)
			x.push_back(ACStart+sign(ACEnd-ACStart)*i*abs(ACStep));
		ui->resultPlot->xAxis->setScaleType(QCPAxis::stLinear);
	}
	else if(ACSweepType==Calculate::Log)
	{
		ACSweepNum=abs((int)log10(ACStart/ACEnd)*(int)ACStep);
		for(int i=0;i<ACSweepNum;i++)
			x.push_back(ACStart*pow(10.0,sign(ACEnd-ACStart)*abs((double)i/(int)ACStep)));
		ui->resultPlot->xAxis->setScaleType(QCPAxis::stLogarithmic);
	}
	if(parametricSweepType==Calculate::Linear)
		parametricSweepNum=abs((int)((parametricStart-parametricEnd)/parametricStep));
	else if(parametricSweepType==Calculate::Log)
		parametricSweepNum=abs((int)log10(parametricStart/parametricEnd)*(int)parametricStep);
	else
		parametricSweepNum=1;
}

void ACSweepResult::plotCheckedVariables()
{
	//初始化为空视图
	ui->resultPlot->legend->setVisible(false);
	ui->resultPlot->legend->clear();
	for(int i=0;i<ui->resultPlot->plottableCount();i++)
		((QCPCurve *)ui->resultPlot->plottable(i))->data()->clear();
	ui->resultPlot->yAxis->setVisible(false);
	ui->resultPlot->yAxis2->setVisible(false);
	ui->resultPlot->yAxis->setScaleType(QCPAxis::stLinear);
	ui->resultPlot->yAxis2->setScaleType(QCPAxis::stLinear);
	if(ui->resultPlot->axisRect()->axisCount(QCPAxis::atLeft)==2)
	{
		QCPAxis *phaseAxis=ui->resultPlot->axisRect()->axis(QCPAxis::atLeft,1);
		phaseAxis->setVisible(false);
		phaseAxis->setScaleType(QCPAxis::stLinear);
	}

	//设置是否对数坐标
	if(ui->yAxisLog->checkState()==Qt::Checked)
		ui->resultPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
	if(ui->yAxis2Log->checkState()==Qt::Checked)
		ui->resultPlot->yAxis2->setScaleType(QCPAxis::stLogarithmic);

	//绘制图像
	int no=0;
	for(int i=0;i<ui->variableList->count();i++)
		if(ui->variableList->item(i)->checkState()==Qt::Checked)
			if(parametricSweepType==Calculate::Unabled)
				plotCurve(ui->variableList->item(i)->text(),no++,0);
			else if(parametricSweepType==Calculate::Linear)
			{
				for(int j=0;j<parametricSweepNum;j++)
				{
					parametricPart->setParameter(parametricParameterNo,parametricStart+sign(parametricEnd-parametricStart)*j*abs(parametricStep));
					plotCurve(ui->variableList->item(i)->text(),no,j);
				}
				no++;
			}
			else if(parametricSweepType==Calculate::Log)
			{
				for(int j=0;j<parametricSweepNum;j++)
				{
					parametricPart->setParameter(parametricParameterNo,parametricStart*pow(10.0,sign(parametricEnd-parametricStart)*abs((double)j/(int)parametricStep)));
					plotCurve(ui->variableList->item(i)->text(),no,j);
				}
				no++;
			}
	if(ui->resultPlot->legend->rowCount()>0)
	{
		ui->resultPlot->legend->setVisible(true);
		ui->resultPlot->legend->setFont(QFont("Courier New",10));
		ui->resultPlot->legend->setTextColor(Qt::white);
		ui->resultPlot->legend->setBrush(Qt::black);
		ui->resultPlot->legend->setBorderPen(QPen(Qt::white,1));
		ui->resultPlot->yAxis->grid()->setPen(QPen(QColor(0,0,0,0)));
	}
	ui->resultPlot->rescaleAxes();
	ui->resultPlot->replot();
}

void ACSweepResult::plotCurve(QString curveName,int curveOnScreenNo,int parametricIndex)
{
	QStringList sections=curveName.split(QRegExp("[\(:\)]"));
	if(sections.first()=="V"&&plot(ui->resultPlot->yAxis,curveOnScreenNo,curveName,parametricIndex,VOLTAGE))
		ui->resultPlot->yAxis->setLabel("Voltage(V)");
	else if(sections.first()=="I"&&plot(ui->resultPlot->yAxis2,curveOnScreenNo,curveName,parametricIndex,CURRENT))
		ui->resultPlot->yAxis2->setLabel("Current(A)");
	else if(sections.first()=="Phase")
	{
		if(sections[1]=="V")
		{
			QCPAxis *phaseAxis=ui->resultPlot->axisRect()->axis(QCPAxis::atLeft,1);
			if(plot(phaseAxis,curveOnScreenNo,curveName,parametricIndex,VOLTAGE|PHASE))
				phaseAxis->setLabel(QStringLiteral("Phase(°)"));
		}
		else if(sections[1]=="I")
		{
			QCPAxis *phaseAxis=ui->resultPlot->axisRect()->axis(QCPAxis::atLeft,1);
			if(plot(phaseAxis,curveOnScreenNo,curveName,parametricIndex,CURRENT|PHASE))
				phaseAxis->setLabel(QStringLiteral("Phase(°)"));
		}
	}
}

bool ACSweepResult::plot(QCPAxis *axis,int curveOnScreenNo,QString curveName,int parametricIndex,int variableType)
{
	QStringList sections=curveName.split(QRegExp("[\(:\)]"));
	if(variableType&PHASE)
		sections.removeFirst();
	Element *part=NULL;
	for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
        if(QString((*it)->name.c_str())==sections[1])
		{//假设无重名元件
			part=(*it);
			break;
		}
	if(part==NULL)
		return false;
	QCPCurve *curve=new QCPCurve(ui->resultPlot->xAxis,axis);

	QVector<QCPCurveData> data;//设置数据
	for(int i=0;i<ACSweepNum;i++)
	{
		QCoreApplication::processEvents();//计算过程中保持程序响应
		if(ACSweepType==Calculate::Linear)
		{
			double frequency=ACStart+sign(ACEnd-ACStart)*i*abs(ACStep);
			for(fList<VAC *>::iterator it=ACParts.begin();it!=ACParts.end();++it)
				(*it)->frequency=frequency;
			calculate[parametricIndex].setACSweepOptions(frequency);
			data.push_back(QCPCurveData(i,x[i],calculate[parametricIndex].showResult(part,sections[2].toInt(),variableType,structure,i,
                (double)calculate[parametricIndex].ACSweepPeriodNum/(double)calculate[parametricIndex].ACSweepPoints/frequency)));
		}
		else if(ACSweepType==Calculate::Log)
		{
			double frequency=ACStart*pow(10.0,sign(ACEnd-ACStart)*abs((double)i/(int)ACStep));
			for(fList<VAC *>::iterator it=ACParts.begin();it!=ACParts.end();++it)
				(*it)->frequency=frequency;
			calculate[parametricIndex].setACSweepOptions(frequency);
			data.push_back(QCPCurveData(i,x[i],calculate[parametricIndex].showResult(part,sections[2].toInt(),variableType,structure,i,
                (double)calculate[parametricIndex].ACSweepPeriodNum/(double)calculate[parametricIndex].ACSweepPoints/frequency)));
		}
	}
	curve->data()->set(data,true);//添加数据
	QColor curveColor=Qt::GlobalColor(Qt::red+(curveOnScreenNo%(Qt::yellow-Qt::red)));//设置曲线颜色
	int h=0,s=0,v=0;//设置曲线颜色
	curveColor.getHsv(&h,&s,&v);
	s-=(int)(200.0*(double)parametricIndex/(double)parametricSweepNum);
	curveColor.setHsv(h,s,v);
    curve->setPen(QPen(curveColor,2));
	axis->setVisible(true);
	axis->setTickLabels(true);
	axis->setLabelColor(Qt::white);
	axis->setBasePen(QPen(Qt::white));
	axis->setLabelFont(QFont("Courier New",12));
	axis->setSubTickPen(QPen(Qt::white));
	axis->setTickPen(QPen(Qt::white));
	axis->setTickLabelColor(Qt::white);
	curve->setName(curveName);//设置曲线名
	if(parametricIndex!=0)
		curve->removeFromLegend();
	return true;
}

void ACSweepResult::searchVariable(QString variable)
{
	QList<QListWidgetItem *> searchItems=ui->variableList->findItems(variable,Qt::MatchContains);
	for(int i=0;i<ui->variableList->count();i++)
		ui->variableList->setRowHidden(i,true);//隐藏所有行
	for(int i=0;i<searchItems.count();i++)
		ui->variableList->setItemHidden(searchItems.at(i),false);//显示查找到的信息
}

ACSweepResult::~ACSweepResult()
{
	delete []calculate;
	delete structure;
    delete ui;
}
