#ifndef SIMULATE_H
#define SIMULATE_H
#include "matrix.h"
#include "element.h"
#include "mainwindow.h"

const double ITERATION_ABSOLUTE_ERROR=1.0e-9;
const double ITERATION_RELATIVE_ERROR=1.0e-3;
const double GMIN_START=1.0;
const double GMIN_END=1.0e-12;
const int MAX_ITERATION_COUNT=100;
const int VOLTAGE=0x0;
const int CURRENT=0x1;
const int TYPE=0x1;
const int PHASE=0x2;

class Structure
{
public:
	fList<Element *> parts;
	bool hasBJT;
	bool hasACSource;
	int fNum;//总方程数（即自变量数）
	bool initData(QString fileName);
	Matrix generateJacobian(Matrix &x,Matrix &oldx,double h,double t);
	Matrix generateFunctions(Matrix &x,Matrix &oldx,double h,double t);
	inline void variableToPartPinIndex(Element *part,Matrix &x,Matrix &oldx,Matrix &partx,Matrix &partOldx);
	Structure():KCLNum(0),fNum(0),hasBJT(false),hasACSource(false){}
	~Structure();
private:
	int KCLNum;//KCL所对应的方程数
};

class Calculate
{
public:
	virtual double showResult(Element *part,int pinIndex,int variableType,Structure *structure,int index,double h)=0;
	enum sweepType{Unabled,Linear,Log};//扫描类型
protected:
	void calculateInit(Matrix &variable,Structure *structure,double h=1.0e-6);
	void NewtonRaphsonMethod(Matrix &x,Matrix &oldx,double h,double t,Structure *structure);
};

class TimeDomainCalculate : public Calculate
{
public:
	void timeDomain(double h,double runTo,Structure *structure,MainWindow *mainwindow);
	virtual double showResult(Element *part,int pinIndex,int variableType,Structure *structure,int index,double h);
	TimeDomainCalculate():variableResult(NULL){}
	~TimeDomainCalculate(){if(variableResult!=NULL) delete []variableResult;}
private:
	Matrix *variableResult;
};

class DCSweepCalculate : public Calculate
{
public:
	void dCSweep(Element *part,int parameterNo,double primaryStart,double primaryEnd,double primaryStep,int primarySweepType,Structure *structure,MainWindow *mainwindow);
	virtual double showResult(Element *part,int pinIndex,int variableType,Structure *structure,int index,double h=1.0e-6);
	DCSweepCalculate():variableResult(NULL){}
	~DCSweepCalculate(){if(variableResult!=NULL) delete []variableResult;}
private:
	Matrix *variableResult;
};

class ACSweepCalculate : public Calculate
{
public:
	int ACSweepPoints;
	int ACSweepPeriodNum;
    void setACSweepOptions(double frequency);
	void aCSweep(fList<VAC *> VACParts,double ACStart,double ACEnd,double ACStep,int ACSweepType,Structure *structure,MainWindow *mainwindow);
	virtual double showResult(Element *part,int pinIndex,int variableType,Structure *structure,int index,double h);
	ACSweepCalculate():variableResult(NULL){}
	~ACSweepCalculate()
	{
		if(variableResult!=NULL)
		{
			for(int i=0;i<ACSweepNum;i++)
				delete []variableResult[i];
			delete []variableResult;
		}
	}
private:
	Matrix **variableResult;
	int ACSweepNum;
	double peakValue(int index,int variableNo);
	double peakValue(Matrix &result);
	double phase(int index,int variableNo);
	double phase(Matrix &result);
};

#endif // SIMULATE_H
