#include "element.h"
#include "simulate.h"
#include <fstream>
#include <QProgressBar>
#include <QStatusBar>
#include <QCoreApplication>

void TimeDomainCalculate::timeDomain(double h,double runTo,Structure *structure,MainWindow *mainwindow)
{
	variableResult=new Matrix[(int)(runTo/h)+1];
	QProgressBar *simulationProgress=new QProgressBar;//设置进度条
	simulationProgress->setMaximumSize(200,20);
	mainwindow->statusBar()->addWidget(simulationProgress);
	simulationProgress->setRange(0,(int)(runTo/h));
	simulationProgress->setValue(0);
	QCoreApplication::processEvents();//计算过程中保持程序响应
	
	calculateInit(variableResult[0],structure,h);//估算迭代初始值
	for(int i=1;i<=(int)(runTo/h);i++)
	{
		variableResult[i]=variableResult[i-1];
		NewtonRaphsonMethod(variableResult[i],variableResult[i-1],h,i*h,structure);
		QCoreApplication::processEvents();//计算过程中保持程序响应
		simulationProgress->setValue(i);
	}
	mainwindow->statusBar()->removeWidget(simulationProgress);
	delete simulationProgress;
}

void DCSweepCalculate::dCSweep(Element *part,int parameterNo,double primaryStart,double primaryEnd,double primaryStep,int primarySweepType,Structure *structure,MainWindow *mainwindow)
{
	int primarySweepNum;
	QProgressBar *simulationProgress=new QProgressBar;//设置进度条
	simulationProgress->setMaximumSize(200,20);
	mainwindow->statusBar()->addWidget(simulationProgress);
	simulationProgress->setValue(0);
	QCoreApplication::processEvents();//计算过程中保持程序响应

	if(primarySweepType==Linear)
	{
		primarySweepNum=abs((int)((primaryEnd-primaryStart)/primaryStep));
		variableResult=new Matrix[primarySweepNum];
		simulationProgress->setRange(0,primarySweepNum);
		for(int i=0;i<primarySweepNum;i++)
		{
			part->setParameter(parameterNo,primaryStart+sign(primaryEnd-primaryStart)*i*abs(primaryStep));
			calculateInit(variableResult[i],structure);
			QCoreApplication::processEvents();//计算过程中保持程序响应
			simulationProgress->setValue(i);
		}
	}
	else if(primarySweepType==Log)
	{
		primarySweepNum=abs((int)log10(primaryEnd/primaryStart)*(int)primaryStep);
		variableResult=new Matrix[primarySweepNum];
		simulationProgress->setRange(0,primarySweepNum);
		for(int i=0;i<primarySweepNum;i++)
		{
			part->setParameter(parameterNo,primaryStart*pow(10.0,sign(primaryEnd-primaryStart)*abs((double)i/(int)primaryStep)));
			calculateInit(variableResult[i],structure);
			QCoreApplication::processEvents();
			simulationProgress->setValue(i);
		}
	}

	mainwindow->statusBar()->removeWidget(simulationProgress);
	delete simulationProgress;
}

void ACSweepCalculate::aCSweep(fList<VAC *> VACParts,double ACStart,double ACEnd,double ACStep,int ACSweepType,Structure *structure,MainWindow *mainwindow)
{
	QProgressBar *simulationProgress=new QProgressBar;//设置进度条
	simulationProgress->setMaximumSize(200,20);
	mainwindow->statusBar()->addWidget(simulationProgress);
	simulationProgress->setValue(0);
	QCoreApplication::processEvents();//计算过程中保持程序响应

	if(ACSweepType==Calculate::Linear)
	{
		ACSweepNum=abs((int)((ACEnd-ACStart)/ACStep));
		variableResult=new Matrix*[ACSweepNum];
		for(int i=0;i<ACSweepNum;i++)
		{
			setACSweepOptions(ACStart+sign(ACEnd-ACStart)*i*abs(ACStep));
			variableResult[i]=new Matrix[ACSweepPoints];
		}
		simulationProgress->setRange(0,ACSweepNum);
		Matrix jacInverse(structure->fNum,structure->fNum);
		for(int i=0;i<ACSweepNum;i++)
		{
			double frequency=ACStart+sign(ACEnd-ACStart)*i*abs(ACStep);
			for(fList<VAC *>::iterator it=VACParts.begin();it!=VACParts.end();++it)
				(*it)->frequency=frequency;
			setACSweepOptions(frequency);
			double h=ACSweepPeriodNum/ACSweepPoints/frequency;
			calculateInit(variableResult[i][0],structure);
			jacInverse=structure->generateJacobian(variableResult[i][0],variableResult[i][0],h,h).inverse();//获取电路在直流工作点的交流小信号模型
			for(int j=1;j<ACSweepPoints;j++)
				variableResult[i][j]=variableResult[i][j-1]-jacInverse*structure->generateFunctions(variableResult[i][j-1],variableResult[i][j-1],h,(double)j*h);
			QCoreApplication::processEvents();//计算过程中保持程序响应
			simulationProgress->setValue(i);
		}
	}
	else if(ACSweepType==Calculate::Log)
	{
		ACSweepNum=abs((int)log10(ACEnd/ACStart)*(int)ACStep);
		variableResult=new Matrix*[ACSweepNum];
		for(int i=0;i<ACSweepNum;i++)
		{
			setACSweepOptions(ACStart*pow(10.0,sign(ACEnd-ACStart)*abs((double)i/(int)ACStep)));
			variableResult[i]=new Matrix[ACSweepPoints];
		}
		simulationProgress->setRange(0,ACSweepNum);
		Matrix jacInverse(structure->fNum,structure->fNum);
		for(int i=0;i<ACSweepNum;i++)
		{
			double frequency=ACStart*pow(10.0,sign(ACEnd-ACStart)*abs((double)i/(int)ACStep));
			for(fList<VAC *>::iterator it=VACParts.begin();it!=VACParts.end();++it)
				(*it)->frequency=frequency;
			setACSweepOptions(frequency);
			double h=(double)ACSweepPeriodNum/ACSweepPoints/frequency;
			calculateInit(variableResult[i][0],structure);
			jacInverse=structure->generateJacobian(variableResult[i][0],variableResult[i][0],h,h).inverse();//获取电路在直流工作点的交流小信号模型
			for(int j=1;j<ACSweepPoints;j++)
				variableResult[i][j]=variableResult[i][j-1]-jacInverse*structure->generateFunctions(variableResult[i][j-1],variableResult[i][j-1],h,(double)j*h);
			QCoreApplication::processEvents();
			simulationProgress->setValue(i);
		}
	}
	mainwindow->statusBar()->removeWidget(simulationProgress);
	delete simulationProgress;
}

void Calculate::calculateInit(Matrix &variable,Structure *structure,double h)
{
	//将动态元件置零
	fList<double> parameterTmp;
	fList<double>::iterator dit;
	fList<Element *>::iterator it;
	for(it=structure->parts.begin();it!=structure->parts.end();++it)
        if((((*it)->type=="L")&&(((Inductor *)(*it))->setInitCurrent==false))||(((*it)->type=="C")&&(((Capacitor *)(*it))->setInitVoltage==false)))
		{
			parameterTmp.push_front((*it)->showParameter(0));
			(*it)->setParameter(0,0.0);
		}
	parameterTmp.reverse();
	//计算初始值（直流工作点）
	Matrix init(structure->fNum,1);
	variable=init;//变量在初始时为0
	if(structure->hasBJT)
	{//若有BJT，则利用Gmin估算一个迭代初始值
		for(Element::Gmin=GMIN_START;Element::Gmin>GMIN_END;Element::Gmin/=2.0)
			NewtonRaphsonMethod(variable,init,h,0.0,structure);
	}
	NewtonRaphsonMethod(variable,init,h,0.0,structure);
	//还原动态元件属性
	for(it=structure->parts.begin(),dit=parameterTmp.begin();it!=structure->parts.end();++it)
        if((((*it)->type=="L")&&(((Inductor *)(*it))->setInitCurrent==false))||(((*it)->type=="C")&&(((Capacitor *)(*it))->setInitVoltage==false)))
		{
			(*it)->setParameter(0,*dit);
			++dit;
		}
}

double TimeDomainCalculate::showResult(Element *part,int pinIndex,int variableType,Structure *structure,int index,double h)
{
	if(part->type!="GND")
	{
		if(variableType==VOLTAGE)
			if(part->pin[pinIndex]<0)
				return 0.0;
			else
				return variableResult[index](part->pin[pinIndex],0);
		else if(variableType==CURRENT)
			if(pinIndex>=part->VCBranchNum)
				return variableResult[index](part->CCPinToVariable[pinIndex-part->VCBranchNum],0);
			else
			{
				//将x、oldx中与当前元件相对应的变量临时提取出来，按照元件管脚编号存储至partx、partOldx
				Matrix partx(part->pinNum,1),partOldx(part->pinNum,1);
				if(index==0)
				{
					structure->variableToPartPinIndex(part,variableResult[index],variableResult[index],partx,partOldx);
					return part->f(pinIndex,partx,partOldx,h,0.0);
				}
				else
				{
					structure->variableToPartPinIndex(part,variableResult[index],variableResult[index-1],partx,partOldx);
					return part->f(pinIndex,partx,partOldx,h,index*h);
				}
			}
		else
			return -1.0;//返回错误
	}
	else
		return 0.0;
}

double DCSweepCalculate::showResult(Element *part,int pinIndex,int variableType,Structure *structure,int index,double h)
{
	if(part->type!="GND")
	{
		if(variableType==VOLTAGE)
			if(part->pin[pinIndex]<0)
				return 0.0;
			else
				return variableResult[index](part->pin[pinIndex],0);
		else if(variableType==CURRENT)
			if(pinIndex>=part->VCBranchNum)
				return variableResult[index](part->CCPinToVariable[pinIndex-part->VCBranchNum],0);
			else
			{
				//将x、oldx中与当前元件相对应的变量临时提取出来，按照元件管脚编号存储至partx、partOldx
				Matrix partx(part->pinNum,1),partOldx(part->pinNum,1),oldx(structure->fNum,1);
				structure->variableToPartPinIndex(part,variableResult[index],oldx,partx,partOldx);
				return part->f(pinIndex,partx,partOldx,h,0.0);
			}
		else
			return -1.0;//返回错误
	}
	else
		return 0.0;
}

double ACSweepCalculate::showResult(Element *part,int pinIndex,int variableType,Structure *structure,int index,double h)
{
	if(part->type!="GND")
	{
		if((variableType&TYPE)==VOLTAGE)
			if(part->pin[pinIndex]<0)
				return 0.0;
			else
				if(variableType&PHASE)
					return phase(index,part->pin[pinIndex]);
				else
					return peakValue(index,part->pin[pinIndex]);
		else if((variableType&TYPE)==CURRENT)
			if(pinIndex>=part->VCBranchNum)
				if(variableType&PHASE)
					return phase(index,part->CCPinToVariable[pinIndex-part->VCBranchNum]);
				else
					return peakValue(index,part->CCPinToVariable[pinIndex-part->VCBranchNum]);
			else
			{
				Matrix result(ACSweepPoints/ACSweepPeriodNum,1),partx(part->pinNum,1),partOldx(part->pinNum,1);
				if(part->type=="L"||part->type=="C")//动态元件
					for(int i=0;i<ACSweepPoints/ACSweepPeriodNum;i++)
					{	
						//将x、oldx中与当前元件相对应的变量临时提取出来，按照元件管脚编号存储至partx、partOldx
						structure->variableToPartPinIndex(part,variableResult[index][i+ACSweepPoints-ACSweepPoints/ACSweepPeriodNum],
							variableResult[index][i+ACSweepPoints-ACSweepPoints/ACSweepPeriodNum-1],partx,partOldx);
						result(i,0)=part->f(pinIndex,partx,partOldx,h,(i+ACSweepPoints-ACSweepPoints/ACSweepPeriodNum)*h);
					}
				else
				{//其余元件
					structure->variableToPartPinIndex(part,variableResult[index][0],variableResult[index][0],partx,partOldx);
					double biasResult=part->f(pinIndex,partx,partOldx,h,0.0);//直流工作点
					double *ACModelParameter=new double[part->pinNum];//交流小信号模型的各参数（线性系数）
					for(int i=0;i<part->pinNum;i++)
						ACModelParameter[i]=part->df(pinIndex,i,partx,partOldx,h,0.0);
					for(int i=0,j;i<ACSweepPoints/ACSweepPeriodNum;i++)
					{	
						//将x、oldx中与当前元件相对应的变量临时提取出来，按照元件管脚编号存储至partx、partOldx
						structure->variableToPartPinIndex(part,variableResult[index][i+ACSweepPoints-ACSweepPoints/ACSweepPeriodNum],
							variableResult[index][i+ACSweepPoints-ACSweepPoints/ACSweepPeriodNum-1],partx,partOldx);
						for(j=0,result(i,0)=biasResult;j<part->pinNum;j++)//以交流小信号模型近似的结果
							result(i,0)+=partx(j,0)*ACModelParameter[j];
					}
				}
				if(variableType&PHASE)
					return phase(result);
				else
					return peakValue(result);
			}
		else
			return -1.0;//返回错误
	}
	else
		return 0.0;
}

void Calculate::NewtonRaphsonMethod(Matrix &x,Matrix &oldx,double h,double t,Structure *structure)
{
	int iterationCount=0;
	Matrix lastx(x.rowCount(),x.columnCount());
	bool loop=false;
	do
	{
		iterationCount++;
		loop=false;
		lastx=x;
		x=x-structure->generateJacobian(x,oldx,h,t).inverse()*structure->generateFunctions(x,oldx,h,t);//迭代
		for(int i=0;i<x.rowCount();i++)
			if(abs(x(i,0)-lastx(i,0))>ITERATION_ABSOLUTE_ERROR+abs(x(i,0))*ITERATION_RELATIVE_ERROR)
			{
				loop=true;
				break;
			}
	}while(loop&&iterationCount<MAX_ITERATION_COUNT);
}

double ACSweepCalculate::peakValue(int index,int variableNo)
{
	Matrix tmp(ACSweepPoints/ACSweepPeriodNum,1);
	for(int i=0;i<ACSweepPoints/ACSweepPeriodNum;i++)
		tmp(i,0)=variableResult[index][i+ACSweepPoints-ACSweepPoints/ACSweepPeriodNum](variableNo,0);
	return peakValue(tmp);
}

double ACSweepCalculate::peakValue(Matrix &result)
{
	Matrix tmp=result.FFT();
	return sqrt(pow(tmp(1,0),2)+pow(tmp(1,1),2))*2.0*(double)ACSweepPeriodNum/ACSweepPoints;
}

double ACSweepCalculate::phase(int index,int variableNo)
{
	Matrix tmp(ACSweepPoints/ACSweepPeriodNum,1);
	for(int i=0;i<ACSweepPoints/ACSweepPeriodNum;i++)
		tmp(i,0)=variableResult[index][i+ACSweepPoints-ACSweepPoints/ACSweepPeriodNum](variableNo,0);
	return phase(tmp);
}

double ACSweepCalculate::phase(Matrix &result)
{
	Matrix tmp=result.FFT();
	if(tmp(1,1)<0.0)
		return (-atan(tmp(1,0)/tmp(1,1))*180.0/PI);
	else
		return (180.0-atan(tmp(1,0)/tmp(1,1))*180.0/PI);
}

void ACSweepCalculate::setACSweepOptions(double frequency)
{
	int nextPow2=(int)(log(frequency)/log(2))+1;
	if(nextPow2<8)
		ACSweepPeriodNum=4;
	else if(nextPow2<20)
		ACSweepPeriodNum=1<<(nextPow2-5);
	else
		ACSweepPeriodNum=1<<15;
	if(nextPow2<14)
		ACSweepPoints=ACSweepPeriodNum<<7;
	else if(nextPow2<17)
		ACSweepPoints=1<<16;
	else if(nextPow2<20)
		ACSweepPoints=ACSweepPeriodNum<<5;
	else
		ACSweepPoints=1<<20;
}

bool Structure::initData(QString fileName)
{
	//初始化元件链表parts
	stringstream staticNo;
	Element::showStaticNo(staticNo);
    ifstream infile(fileName.toStdString(),ios::in);
	if(!infile)
		return false;
	string tmpType;
	string tmpParameter;
	Element *tmpPart;
	while(infile>>tmpType)
	{
		if((tmpPart=Element::newElement(tmpType))!=NULL)
		{//如果对象创建成功，则依次录入信息
			infile>>tmpPart->name>>*tmpPart;
			parts.push_front(tmpPart);
		}
		else//否则跳过该行
			getline(infile,tmpType);
	}
	Element::setStaticNo(staticNo);
	//初始化各元件的pin、CCPinToVariable
	KCLNum=0;
	for(fList<Element *>::iterator it=parts.begin();it!=parts.end();++it)
		for(int i=0;i<(*it)->pinNum;i++)
			if(--((*it)->pin[i])>KCLNum)//将管脚编号均自减1
				KCLNum=(*it)->pin[i];
	fNum=++KCLNum;
	for(fList<Element *>::iterator it=parts.begin();it!=parts.end();++it)
		for(int i=0;i<(*it)->pinNum-(*it)->VCBranchNum;i++)
			(*it)->CCPinToVariable[i]=fNum++;
	//判断电路中是否有BJT及交流源（VAC、VSin、VPulse等）
	hasBJT=hasACSource=false;
	for(fList<Element *>::iterator it=parts.begin();it!=parts.end()&&(!hasBJT||!hasACSource);++it)
		if((*it)->type=="NPN"||(*it)->type=="PNP")
			hasBJT=true;
		else if(((*it)->type[0]=='V'&&(*it)->type!="VDC")||((*it)->type[0]=='I'&&(*it)->type!="IDC"))
			hasACSource=true;
	return true;
}

Matrix Structure::generateJacobian(Matrix &x,Matrix &oldx,double h,double t)
{
	if(fNum!=x.rowCount())
		return Matrix(0,0);//若矩阵行数不等于列数，则返回空矩阵
	Matrix jac(fNum,x.rowCount()),partx(MAX_PIN_NUM,1),partOldx(MAX_PIN_NUM,1);
	int l,j,k,lToVariable,GOLIndex=KCLNum;//GOLIndex：当前所生成GOL的序号
	for(fList<Element *>::iterator it=parts.begin();it!=parts.end();++it)
	{
		//将x、oldx中与当前元件相对应的变量临时提取出来，按照元件管脚编号存储至partx、partOldx
		variableToPartPinIndex((*it),x,oldx,partx,partOldx);
		//构建Jacobian矩阵
		//当前元件对第pin[l]节点KCL的贡献
		for(l=0;l<(*it)->VCBranchNum;l++)
			if((*it)->pin[l]>=0)
			{
				lToVariable=(*it)->pin[l];
				for(j=0;j<(*it)->VCBranchNum;j++)
					if((*it)->pin[j]>=0)
						jac(lToVariable,(*it)->pin[j])+=(*it)->df(l,j,partx,partOldx,h,t);
				for(;j<(*it)->pinNum;j++)
					jac(lToVariable,(*it)->CCPinToVariable[j-(*it)->VCBranchNum])+=(*it)->df(l,j,partx,partOldx,h,t);
			}
		for(;l<(*it)->pinNum;l++)
			if((*it)->pin[l]>=0)
				jac((*it)->pin[l],(*it)->CCPinToVariable[l-(*it)->VCBranchNum])+=1.0;
		//当前元件对GOL的贡献
		for(k=(*it)->VCBranchNum;k<(*it)->pinNum&&GOLIndex<fNum;k++,GOLIndex++)
		{
			if((*it)->pin[k]>=0)
				jac(GOLIndex,(*it)->pin[k])=1.0;
			for(l=0;l<(*it)->VCBranchNum;l++)
				if((*it)->pin[l]>=0)
					jac(GOLIndex,(*it)->pin[l])= -(*it)->df(k,l,partx,partOldx,h,t);
			for(;l<(*it)->pinNum;l++)
				jac(GOLIndex,(*it)->CCPinToVariable[l-(*it)->VCBranchNum])= -(*it)->df(k,l,partx,partOldx,h,t);
		}
	}
	return jac;
}

Matrix Structure::generateFunctions(Matrix &x,Matrix &oldx,double h,double t)
{
	Matrix func(fNum,1),partx(MAX_PIN_NUM,1),partOldx(MAX_PIN_NUM,1);
	int l,k,GOLIndex=KCLNum;//GOLIndex：当前所生成GOL的序号
	for(fList<Element *>::iterator it=parts.begin();it!=parts.end();++it)
	{
		//将x、oldx中与当前元件相对应的变量临时提取出来，按照元件管脚编号存储至partx、partOldx
		variableToPartPinIndex((*it),x,oldx,partx,partOldx);
		//构建func
		//当前元件对KCL的贡献
		for(l=0;l<(*it)->VCBranchNum;l++)
			if((*it)->pin[l]>=0)
				func((*it)->pin[l],0)+=(*it)->f(l,partx,partOldx,h,t);
		for(;l<(*it)->pinNum;l++)
			if((*it)->pin[l]>=0)
				func((*it)->pin[l],0)+=x((*it)->CCPinToVariable[l-(*it)->VCBranchNum],0);
		//当前元件对GOL的贡献
		for(k=(*it)->VCBranchNum;k<(*it)->pinNum&&GOLIndex<fNum;k++,GOLIndex++)
			if((*it)->pin[k]>=0)
				func(GOLIndex,0)=x((*it)->pin[k],0)-(*it)->f(k,partx,partOldx,h,t);
			else
				func(GOLIndex,0)= -(*it)->f(k,partx,partOldx,h,t);
	}
	return func;
}

void Structure::variableToPartPinIndex(Element *part,Matrix &x,Matrix &oldx,Matrix &partx,Matrix &partOldx)
{
	int l,lToVariable;
	for(l=0;l<part->VCBranchNum;l++)
		if(part->pin[l]<0)//若为地节点，则该点电压为0
			partx(l,0)=partOldx(l,0)=0.0;
		else
		{
			lToVariable=part->pin[l];
			partx(l,0)=x(lToVariable,0);
			partOldx(l,0)=oldx(lToVariable,0);
		}
	for(;l<part->pinNum;l++)
	{
		lToVariable=part->CCPinToVariable[l-part->VCBranchNum];
		partx(l,0)=x(lToVariable,0);
		partOldx(l,0)=oldx(lToVariable,0);
	}
}

Structure::~Structure()
{
	for(fList<Element *>::iterator it=parts.begin();it!=parts.end();++it)
		delete *it;
	parts.clear();
}
