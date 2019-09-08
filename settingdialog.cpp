#include "settingdialog.h"
#include "ui_simulationsetting.h"
#include <QMessageBox>

SimulationSetting::SimulationSetting(Structure *_structure,QWidget *parent) :
    structure(_structure),QDialog(parent), 
    ui(new Ui::SimulationSetting)
{
    ui->setupUi(this);
	ui->simulationSettingWidget->setCurrentIndex(0);
	ui->DCSweepTabWidget->setCurrentIndex(0);
	ui->ACSweepTabWidget->setCurrentIndex(0);
	ui->simulationType->setFocus();

	//设置扫描元件名及参数名列表
	QListWidgetItem *item=NULL;
	QRadioButton *button=NULL;
	for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
	{
        item=new QListWidgetItem("     "+QString((*it)->name.c_str()),ui->primaryPartList);
		button=new QRadioButton();
		connect(button,SIGNAL(clicked()),this,SLOT(setDCParameterList()));
		ui->primaryPartList->setItemWidget(item,button);
        item=new QListWidgetItem("     "+QString((*it)->name.c_str()),ui->secondaryPartList);
		button=new QRadioButton();
		connect(button,SIGNAL(clicked()),this,SLOT(setDCParameterList()));
		ui->secondaryPartList->setItemWidget(item,button);
        item=new QListWidgetItem("     "+QString((*it)->name.c_str()),ui->parametricPartList);
		button=new QRadioButton();
		connect(button,SIGNAL(clicked()),this,SLOT(setACParameterList()));
		ui->parametricPartList->setItemWidget(item,button);
	}
	((QRadioButton *)ui->primaryPartList->itemWidget(ui->primaryPartList->item(0)))->setChecked(true);
	((QRadioButton *)ui->secondaryPartList->itemWidget(ui->secondaryPartList->item(0)))->setChecked(true);
	((QRadioButton *)ui->parametricPartList->itemWidget(ui->parametricPartList->item(0)))->setChecked(true);
	setDCParameterList();
}

void SimulationSetting::setDCPrimaryStepLabel()
{
	if(ui->primaryLinear->isChecked())
		ui->primaryStepLabel->setText(QStringLiteral("增量："));
	else if(ui->primaryLog->isChecked())
		ui->primaryStepLabel->setText(QStringLiteral("每十倍样点数："));
}

void SimulationSetting::setDCSecondaryStepLabel()
{
	if(ui->secondaryLinear->isChecked())
		ui->secondaryStepLabel->setText(QStringLiteral("增量："));
	else if(ui->secondaryLog->isChecked())
		ui->secondaryStepLabel->setText(QStringLiteral("每十倍样点数："));
}

void SimulationSetting::setACParametricStepLabel()
{
	if(ui->parametricLinear->isChecked())
		ui->parametricStepLabel->setText(QStringLiteral("增量："));
	else if(ui->parametricLog->isChecked())
		ui->parametricStepLabel->setText(QStringLiteral("每十倍样点数："));
}

void SimulationSetting::setACStepLabel()
{
	if(ui->ACLinear->isChecked())
		ui->ACStepLabel->setText(QStringLiteral("增量："));
	else if(ui->ACLog->isChecked())
		ui->ACStepLabel->setText(QStringLiteral("每十倍样点数："));
}

void SimulationSetting::searchDCPart(QString partName)
{
	QListWidget *currentList;
	if(ui->DCSweepTabWidget->currentIndex()==0)
		currentList=ui->primaryPartList;
	else if(ui->DCSweepTabWidget->currentIndex()==1)
		currentList=ui->secondaryPartList;
	else
		return;
	QList<QListWidgetItem *> searchItems=currentList->findItems(partName,Qt::MatchContains);
	for(int i=0;i<currentList->count();i++)
		currentList->setRowHidden(i,true);//隐藏所有行
	for(int i=0;i<searchItems.count();i++)
		currentList->setItemHidden(searchItems.at(i),false);//显示查找到的信息
}

void SimulationSetting::searchACPart(QString partName)
{
	QList<QListWidgetItem *> searchItems=ui->parametricPartList->findItems(partName,Qt::MatchContains);
	for(int i=0;i<ui->parametricPartList->count();i++)
		ui->parametricPartList->setRowHidden(i,true);//隐藏所有行
	for(int i=0;i<searchItems.count();i++)
		ui->parametricPartList->setItemHidden(searchItems.at(i),false);//显示查找到的信息
}

void SimulationSetting::searchDCParameter(QString parameterName)
{
	QListWidget *currentList;
	if(ui->DCSweepTabWidget->currentIndex()==0)
		currentList=ui->primaryParameterList;
	else if(ui->DCSweepTabWidget->currentIndex()==1)
		currentList=ui->secondaryParameterList;
	else
		return;
	QList<QListWidgetItem *> searchItems=currentList->findItems(parameterName,Qt::MatchContains);
	for(int i=0;i<currentList->count();i++)
		currentList->setRowHidden(i,true);//隐藏所有行
	for(int i=0;i<searchItems.count();i++)
		currentList->setItemHidden(searchItems.at(i),false);//显示查找到的信息
}

void SimulationSetting::searchACParameter(QString parameterName)
{
	QList<QListWidgetItem *> searchItems=ui->parametricParameterList->findItems(parameterName,Qt::MatchContains);
	for(int i=0;i<ui->parametricParameterList->count();i++)
		ui->parametricParameterList->setRowHidden(i,true);//隐藏所有行
	for(int i=0;i<searchItems.count();i++)
		ui->parametricParameterList->setItemHidden(searchItems.at(i),false);//显示查找到的信息
}

void SimulationSetting::setDCParameterList()
{
	QListWidget *currentList;
	QListWidget *currentPartList;
	if(ui->DCSweepTabWidget->currentIndex()==0)
	{
		currentList=ui->primaryParameterList;
		currentPartList=ui->primaryPartList;
	}
	else if(ui->DCSweepTabWidget->currentIndex()==1)
	{
		currentList=ui->secondaryParameterList;
		currentPartList=ui->secondaryPartList;
	}
	else
		return;
	currentList->clear();
	int checkedItem;
	for(checkedItem=0;checkedItem<currentPartList->count();checkedItem++)
		if(((QRadioButton *)currentPartList->itemWidget(currentPartList->item(checkedItem)))->isChecked())
			break;
	QListWidgetItem *item=NULL;
	QRadioButton *button=NULL;
	for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
        if(QString((*it)->name.c_str())==currentPartList->item(checkedItem)->text().trimmed())
		{//假设无重名元件
			for(int i=0;i<(*it)->parameterNum;i++)
			{
                item=new QListWidgetItem("     "+QString::fromLocal8Bit((*it)->showParameterViewName(i)),currentList);
				button=new QRadioButton();
				button->setObjectName(QString::number(i));
				currentList->setItemWidget(item,button);
			}
			((QRadioButton *)currentList->itemWidget(currentList->item(0)))->setChecked(true);
			break;
		}
}

void SimulationSetting::setACParameterList()
{
	ui->parametricParameterList->clear();
	int checkedItem;
	for(checkedItem=0;checkedItem<ui->parametricPartList->count();checkedItem++)
		if(((QRadioButton *)ui->parametricPartList->itemWidget(ui->parametricPartList->item(checkedItem)))->isChecked())
			break;
	QListWidgetItem *item=NULL;
	QRadioButton *button=NULL;
	for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
        if(QString((*it)->name.c_str())==ui->parametricPartList->item(checkedItem)->text().trimmed())
		{//假设无重名元件
			for(int i=0;i<(*it)->parameterNum;i++)
			{
                item=new QListWidgetItem("     "+QString::fromLocal8Bit((*it)->showParameterViewName(i)),ui->parametricParameterList);
				button=new QRadioButton();
				button->setObjectName(QString::number(i));
				ui->parametricParameterList->setItemWidget(item,button);
			}
			((QRadioButton *)ui->parametricParameterList->itemWidget(ui->parametricParameterList->item(0)))->setChecked(true);
			break;
		}
}

void SimulationSetting::enableSecondarySweep(bool enable)
{
	if(enable)
	{
		ui->secondarySweepVariable->setEnabled(true);
		ui->secondarySweepType->setEnabled(true);
		setDCParameterList();
	}
	else
	{
		ui->secondarySweepVariable->setEnabled(false);
		ui->secondarySweepType->setEnabled(false);
	}
}

void SimulationSetting::enableParametricSweep(bool enable)
{
	if(enable)
	{
		ui->parametricSweepVariable->setEnabled(true);
		ui->parametricSweepType->setEnabled(true);
		setACParameterList();
	}
	else
	{
		ui->parametricSweepVariable->setEnabled(false);
		ui->parametricSweepType->setEnabled(false);
	}
}

void SimulationSetting::checkInput()
{//检查输入合法性
	switch(ui->simulationType->currentIndex())
	{
	case 0:
		{//时域分析
			if(etof(ui->runTo->text().toStdString())<-EPS||etof(ui->runTo->text().toStdString())<etof(ui->start->text().toStdString())-EPS)
			{
				QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("“运行到”时间需小于“开始保存数据”时间"));
				return;
			}
			if(ui->step->text()!=""&&etof(ui->step->text().toStdString())<EPS)
			{
				QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("时间步长不能为零"));
				return;
			}
			break;
		}
	case 1:
		{//直流扫描分析
			if(ui->primaryLog->isChecked())
			{
				if(etof(ui->primaryStart->text().toStdString())<EPS)
				{
					QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("主扫描：对数模式下，主扫描“开始值”需大于0"));
					return;
				}
				if(etof(ui->primaryEnd->text().toStdString())<EPS)
				{
					QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("主扫描：对数模式下，主扫描“终止值”需大于0"));
					return;
				}
				if(etof(ui->primaryStep->text().toStdString())<EPS)
				{
					QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("主扫描：对数模式下，主扫描“每十倍样点数”需为正整数"));
					return;
				}
			}
			else if(ui->primaryLinear->isChecked())
			{
				if(abs(etof(ui->primaryStep->text().toStdString()))<EPS)
				{
					QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("主扫描：线性模式下，主扫描“增量”不能为零"));
					return;
				}
			}
			if(ui->enableSecondarySweep->isChecked())
			{
				if(ui->secondaryLog->isChecked())
				{
					if(etof(ui->secondaryStart->text().toStdString())<EPS)
					{
						QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("副扫描：对数模式下，副扫描“开始值”需大于0"));
						return;
					}
					if(etof(ui->secondaryEnd->text().toStdString())<EPS)
					{
						QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("副扫描：对数模式下，副扫描“终止值”需大于0"));
						return;
					}
					if(etof(ui->secondaryStep->text().toStdString())<EPS)
					{
						QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("副扫描：对数模式下，副扫描“每十倍样点数”需为正整数"));
						return;
					}
				}
				else if(ui->secondaryLinear->isChecked())
				{
					if(abs(etof(ui->secondaryStep->text().toStdString()))<EPS)
					{
						QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("副扫描：线性模式下，副扫描“增量”不能为零"));
						return;
					}
				}
				QString primaryCheckedPartName,secondaryCheckedPartName;
				int primaryCheckedParameterNo=0,secondaryCheckedParameterNo=0;
				for(int i=0;i<ui->primaryPartList->count();i++)//获取主扫描元件名
					if(((QRadioButton *)ui->primaryPartList->itemWidget(ui->primaryPartList->item(i)))->isChecked())
					{
						primaryCheckedPartName=ui->primaryPartList->item(i)->text().trimmed();
						break;
					}
				for(int i=0;i<ui->primaryParameterList->count();i++)//获取主扫描元件参数名
					if(((QRadioButton *)ui->primaryParameterList->itemWidget(ui->primaryParameterList->item(i)))->isChecked())
					{
						primaryCheckedParameterNo=((QRadioButton *)ui->primaryParameterList
							->itemWidget(ui->primaryParameterList->item(i)))->objectName().toInt();
						break;
					}
				for(int i=0;i<ui->secondaryPartList->count();i++)//获取副扫描元件名
					if(((QRadioButton *)ui->secondaryPartList->itemWidget(ui->secondaryPartList->item(i)))->isChecked())
					{
						secondaryCheckedPartName=ui->secondaryPartList->item(i)->text().trimmed();
						break;
					}
				for(int i=0;i<ui->secondaryParameterList->count();i++)//获取副扫描元件参数名
					if(((QRadioButton *)ui->secondaryParameterList->itemWidget(ui->secondaryParameterList->item(i)))->isChecked())
					{
						secondaryCheckedParameterNo=((QRadioButton *)ui->secondaryParameterList
							->itemWidget(ui->secondaryParameterList->item(i)))->objectName().toInt();
						break;
					}
				if(primaryCheckedPartName==secondaryCheckedPartName&&primaryCheckedParameterNo==secondaryCheckedParameterNo)//保证主、副扫描的元件及参数名不相同
				{
					QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("主、副扫描的元件及参数名应不相同"));
					return;
				}
			}
			break;
		}
	case 2:
		{
			fList<Element *> ACPart;
			for(fList<Element *>::iterator it=structure->parts.begin();it!=structure->parts.end();++it)
				if((*it)->type=="VAC")//获取主扫描元件指针
					ACPart.push_front(*it);
			if(ACPart.empty())
			{
				QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("未找到VAC元件：交流分析需要至少一个VAC元件"));
				return;
			}
			if(ui->ACLog->isChecked())
			{
				if(etof(ui->ACStart->text().toStdString())<EPS)
				{
					QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("主扫描：对数模式下，主扫描“开始值”需大于0"));
					return;
				}
				if(etof(ui->ACEnd->text().toStdString())<EPS)
				{
					QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("主扫描：对数模式下，主扫描“终止值”需大于0"));
					return;
				}
				if(etof(ui->ACStep->text().toStdString())<EPS)
				{
					QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("主扫描：对数模式下，主扫描“每十倍样点数”需为正整数"));
					return;
				}
			}
			else if(ui->ACLinear->isChecked())
			{
				if(abs(etof(ui->ACStep->text().toStdString()))<EPS)
				{
					QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("参数扫描：线性模式下，主扫描“增量”不能为零"));
					return;
				}
			}
			if(ui->enableParametricSweep->isChecked())
			{
				if(ui->parametricLog->isChecked())
				{
					if(etof(ui->parametricStart->text().toStdString())<EPS)
					{
						QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("参数扫描：对数模式下，副扫描“开始值”需大于0"));
						return;
					}
					if(etof(ui->parametricEnd->text().toStdString())<EPS)
					{
						QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("参数扫描：对数模式下，副扫描“终止值”需大于0"));
						return;
					}
					if(etof(ui->parametricStep->text().toStdString())<EPS)
					{
						QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("参数扫描：对数模式下，副扫描“每十倍样点数”需为正整数"));
						return;
					}
				}
				else if(ui->parametricLinear->isChecked())
				{
					if(abs(etof(ui->parametricStep->text().toStdString()))<EPS)
					{
						QMessageBox::critical(this,QStringLiteral("错误提示"),QStringLiteral("参数扫描：线性模式下，副扫描“增量”不能为零"));
						return;
					}
				}
			}
		}
	}
	accept();
}

SimulationSetting::~SimulationSetting()
{
    delete ui;
}
