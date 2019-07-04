#ifndef RESULTDIALOG
#define RESULTDIALOG
#include <QDialog>
#include "flist.h"
#include "element.h"
#include "simulate.h"
#include "qcustomplot.h"

namespace Ui {
class TimeDomainResult;
class DCSweepResult;
class ACSweepResult;
}

class TimeDomainResult : public QDialog
{
    Q_OBJECT

public:
    explicit TimeDomainResult(double _runTo,double _start,double _h,TimeDomainCalculate *_calculate,Structure *_structure,QWidget *parent=0);
    ~TimeDomainResult();

public slots:
	void plotCheckedVariables();
	void setXAxisVariable(QListWidgetItem *item);
	void searchVariable(QString );
	void fftSetting();

private:
	double h;
	double start;
	double runTo;
	TimeDomainCalculate *calculate;
	Structure *structure;
    Ui::TimeDomainResult *ui;
	void plotCurve(QVector<double> &x,QString curveName,int curveOnScreenNo);
};

class DCSweepResult : public QDialog
{
    Q_OBJECT

public:
    explicit DCSweepResult(DCSweepCalculate *_calculate,Structure *_structure,
		int _primarySweepType,double _primaryStart,double _primaryEnd,double _primaryStep,Element *_primaryPart,int _primaryParameterNo,
		int _secondarySweepType=Calculate::Unabled,double _secondaryStart=0.0,double _secondaryEnd=0.0,double _secondaryStep=0.0,
		Element *_secondaryPart=NULL,int _secondaryParameterNo=0,QWidget *parent=0);
    ~DCSweepResult();

public slots:
	void plotCheckedVariables();
	void searchVariable(QString );

private:
	double primaryStart;
	double primaryEnd;
	double primaryStep;
	int primarySweepType;
	int primarySweepNum;
	Element *primaryPart;
	int primaryParameterNo;
	double secondaryStart;
	double secondaryEnd;
	double secondaryStep;
	int secondarySweepType;
	int secondarySweepNum;
	Element *secondaryPart;
	int secondaryParameterNo;
	DCSweepCalculate *calculate;
	Structure *structure;
	QVector<double> x;//横轴变量
    Ui::DCSweepResult *ui;
	void plotCurve(QString curveName,int curveOnScreenNo,int secondaryIndex);
};

class ACSweepResult : public QDialog
{
    Q_OBJECT

public:
    explicit ACSweepResult(ACSweepCalculate *_calculate,Structure *_structure,
		int _ACSweepType,double _ACStart,double _ACEnd,double _ACStep,fList<VAC *> _ACParts,
		int _parametricSweepType=Calculate::Unabled,double _parametricStart=0.0,double _parametricEnd=0.0,double _parametricStep=0.0,
		Element *_parametricPart=NULL,int _parametricParameterNo=0,QWidget *parent=0);
    ~ACSweepResult();

public slots:
	void plotCheckedVariables();
	void searchVariable(QString );

private:
	double ACStart;
	double ACEnd;
	double ACStep;
	int ACSweepType;
	int ACSweepNum;
	fList<VAC *> ACParts;
	double parametricStart;
	double parametricEnd;
	double parametricStep;
	int parametricSweepType;
	int parametricSweepNum;
	Element *parametricPart;
	int parametricParameterNo;
	ACSweepCalculate *calculate;
	Structure *structure;
	QVector<double> x;//横轴变量
    Ui::ACSweepResult *ui;
	void plotCurve(QString curveName,int curveOnScreenNo,int parametricIndex);
	bool plot(QCPAxis *axis,int curveOnScreenNo,QString curveName,int parametricIndex,int variableType);
};

#endif // RESULTDIALOG_H