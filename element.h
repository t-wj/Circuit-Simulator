#ifndef ELEMENT_H
#define ELEMENT_H

#include <string>
#include <map>
#include <sstream>
#include <cmath>
#include <QPixmap>
#include "matrix.h"
using namespace std;

const int MAX_PIN_NUM=4;

const double PI=4.0*atan(1.0);
const double E=1.602176565e-19;
const double KB=1.3806488e-23;
const double TK=273.15;
inline double etof(string s);
inline string ftoe(double d,int prec=-1);

class Element
{
public:
	string name;//元件名
	string type;//元件种类
	int pinNum;//元件管脚数
	int *pin;//元件管脚所连电路节点编号
	int *CCPinToVariable;//仿真计算用，以存储流控支路所对应计算时的变量编号
	int VCBranchNum;
	//压控支路数，本程序仅考虑可用单值函数表述的元件，或者压控，或者流控，
	//或者以前VCBranchNum个电压、后(pinNum-VCBranchNum)个电流表述的混合控制元件
	//元件约束方程的电流方向规定流入元件为正，写为：
	//	i_n=f(v_0,v_1,...,v_VCBranchNum-1,i_VCBranchNum,...,i_pinNum) (n<VCBranchNum)
	//	v_n=f(v_0,v_1,...,v_VCBranchNum-1,i_VCBranchNum,...,i_pinNum) (n>=VCBranchNum)
	int parameterNum;//参数个数
	static map<string,string> viewNameToType;//显示元件名到元件类型的映射
	static double Gmin;//并联在半导体元件管脚间的小电导，通过逐渐减小其值至允许范围内以获得迭代初值，帮助计算收敛
	
	virtual double f(int pinNo,Matrix &x,Matrix &oldx,double h,double t)=0;//元件约束方程
	virtual double df(int pinNo,int variableNo,Matrix &x,Matrix &oldx,double h,double t)=0;//元件约束方程的偏导数
	void setParameter(int parameterNo,double parameterValue) {parameter[parameterNo]=parameterValue;}
	double showParameter(int parameterNo) const {return parameter[parameterNo];}
    const char *showParameterViewName(int parameterNo) const {return parameterViewName[parameterNo];}
	friend istream &operator>> (istream &input,Element &part);
	friend ostream &operator<< (ostream &output,Element &part);
	Element(string _name="",string _type="",int _pinNum=0,int _VCBranchNum=1,double *_parameter=NULL,int _parameterNum=0)
		:name(_name),type(_type),pinNum(_pinNum),VCBranchNum(_VCBranchNum),parameterNum(_parameterNum),parameterViewName(NULL)
	{
		int i;
		pin=new int[_pinNum];
		if(_pinNum==_VCBranchNum)
			CCPinToVariable=NULL;
		else
			CCPinToVariable=new int[_pinNum-_VCBranchNum];
		for(i=0;i<_pinNum;i++)
			pin[i]=-2;
		for(i=0;i<_pinNum-_VCBranchNum;i++)
			CCPinToVariable[i]=-2;
		parameter=new double[_parameterNum];
		if(_parameter!=NULL)//对元件参数进行初始化；若该数组为空，则在具体元件的初始化中赋以默认值
			for(i=0;i<_parameterNum;i++)
				parameter[i]=_parameter[i];
	}
	virtual ~Element()
	{
		delete []pin;
		delete []parameter;
		if(parameterViewName!=NULL)
			delete []parameterViewName;
		if(CCPinToVariable!=NULL)
			delete []CCPinToVariable;
	}
	
	//静态成员函数：与Element相关的一些函数
	static void mapInit();
	static Element* newElement(string type);
	static QPixmap viewImagePixmap(string type);
	static QPixmap imagePixmap(string type);
	static void setStaticNo(istream &input);
	static void showStaticNo(ostream &output);
protected:
	double f0(Matrix &x,Matrix &oldx,double h,double t)
	{//f0：第0端电流与自变量的关系，由KCL决定
		double tmp=0.0;
		int i;
		for(i=1;i<VCBranchNum;i++)
			tmp-=f(i,x,oldx,h,t);
		for(;i<pinNum;i++)
			tmp-=x(i,0);
		return tmp;
	}
	double df0(int variableNo,Matrix &x,Matrix &oldx,double h,double t)
	{
		double tmp=0.0;
		for(int i=1;i<VCBranchNum;i++)
			tmp-=df(i,variableNo,x,oldx,h,t);
		if(variableNo<VCBranchNum)
			return tmp;
		else
			return (tmp-1.0);
	}
	double *parameter;//元件参数
    const char **parameterViewName;//元件参数名称
};

class TwoTerminalElement : public Element
{
public:
	virtual double f(int pinNo,Matrix &x,Matrix &oldx,double h,double t)
	{
		switch(pinNo)
		{
		case 0:return f0(x,oldx,h,t);break;
		case 1:return f1(x,oldx,h,t);break;
		default:return 0.0;
		}
	}
	virtual double df(int pinNo,int variableNo,Matrix &x,Matrix &oldx,double h,double t)
	{
		switch(pinNo)
		{
		case 0:return df0(variableNo,x,oldx,h,t);break;
		case 1:
			switch(variableNo)
			{
			case 0:return df10(x,oldx,h,t);break;
			case 1:return df11(x,oldx,h,t);break;
			default:return 0.0;
			}break;
		default:return 0.0;
		}
	}
	TwoTerminalElement(string _name="",string _type="",int _VCBranchNum=1,double *_parameter=NULL,int _parameterNum=0)
		:Element(_name,_type,2,_VCBranchNum,_parameter,_parameterNum){}
private:
	virtual double f1(Matrix &x,Matrix &oldx,double h,double t)=0;
	virtual double df10(Matrix &x,Matrix &oldx,double h,double t)=0;
	virtual double df11(Matrix &x,Matrix &oldx,double h,double t)=0;
};

class ThreeTerminalElement : public Element
{
public:
    virtual double f(int pinNo,Matrix &x,Matrix &oldx,double h,double t)
    {
        switch(pinNo)
        {
        case 0:return f0(x,oldx,h,t);break;
        case 1:return f1(x,oldx,h,t);break;
        case 2:return f2(x,oldx,h,t);break;
        default:return 0.0;
        }
    }
    virtual double df(int pinNo,int variableNo,Matrix &x,Matrix &oldx,double h,double t)
    {
        switch(pinNo)
        {
        case 0:return df0(variableNo,x,oldx,h,t);break;
        case 1:
            switch(variableNo)
            {
            case 0:return df10(x,oldx,h,t);break;
            case 1:return df11(x,oldx,h,t);break;
            case 2:return df12(x,oldx,h,t);break;
            default:return 0.0;
            }break;
        case 2:
            switch(variableNo)
            {
            case 0:return df20(x,oldx,h,t);break;
            case 1:return df21(x,oldx,h,t);break;
            case 2:return df22(x,oldx,h,t);break;
            default:return 0.0;
            }break;
        default:return 0.0;
        }
    }
    ThreeTerminalElement(string _name="",string _type="",int _VCBranchNum=1,double *_parameter=NULL,int _parameterNum=0)
        :Element(_name,_type,3,_VCBranchNum,_parameter,_parameterNum){}
private:
    virtual double f1(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double f2(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df10(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df11(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df12(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df20(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df21(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df22(Matrix &x,Matrix &oldx,double h,double t)=0;
};

class FourTerminalElement : public Element
{
public:
    virtual double f(int pinNo,Matrix &x,Matrix &oldx,double h,double t)
    {
        switch(pinNo)
        {
        case 0:return f0(x,oldx,h,t);break;
        case 1:return f1(x,oldx,h,t);break;
        case 2:return f2(x,oldx,h,t);break;
        case 3:return f3(x,oldx,h,t);break;
        default:return 0.0;
        }
    }
    virtual double df(int pinNo,int variableNo,Matrix &x,Matrix &oldx,double h,double t)
    {
        switch(pinNo)
        {
        case 0:return df0(variableNo,x,oldx,h,t);break;
        case 1:
            switch(variableNo)
            {
            case 0:return df10(x,oldx,h,t);break;
            case 1:return df11(x,oldx,h,t);break;
            case 2:return df12(x,oldx,h,t);break;
            case 3:return df13(x,oldx,h,t);break;
            default:return 0.0;
            }break;
        case 2:
            switch(variableNo)
            {
            case 0:return df20(x,oldx,h,t);break;
            case 1:return df21(x,oldx,h,t);break;
            case 2:return df22(x,oldx,h,t);break;
            case 3:return df23(x,oldx,h,t);break;
            default:return 0.0;
            }break;
        case 3:
            switch(variableNo)
            {
            case 0:return df30(x,oldx,h,t);break;
            case 1:return df31(x,oldx,h,t);break;
            case 2:return df32(x,oldx,h,t);break;
            case 3:return df33(x,oldx,h,t);break;
            default:return 0.0;
            }break;
        default:return 0.0;
        }
    }
    FourTerminalElement(string _name="",string _type="",int _VCBranchNum=1,double *_parameter=NULL,int _parameterNum=0)
        :Element(_name,_type,4,_VCBranchNum,_parameter,_parameterNum){}
private:
    virtual double f1(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double f2(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double f3(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df10(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df11(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df12(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df13(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df20(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df21(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df22(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df23(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df30(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df31(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df32(Matrix &x,Matrix &oldx,double h,double t)=0;
    virtual double df33(Matrix &x,Matrix &oldx,double h,double t)=0;
};

class Ground : public Element
{
public:
	virtual double f(int pinNo,Matrix &x,Matrix &oldx,double h,double t)
	{
		return 0.0;
	}
	virtual double df(int pinNo,int variableNo,Matrix &x,Matrix &oldx,double h,double t)
	{
		return 0.0;
	}
	Ground():Element("GND","GND",1,1,NULL,0)
	{
		ostringstream out;
		out<<++No;
		name+=out.str();
	}
	static int No;
};

class Resistor : public TwoTerminalElement
{
public:
	Resistor(double *_parameter=NULL):TwoTerminalElement("R","R",2,_parameter,parameterCount)
	{
		ostringstream out;
		out<<++No;
		name+=out.str();
		if(_parameter==NULL)
			parameter[R]=1000.0;//默认值
        parameterViewName=new const char*[parameterCount];
		parameterViewName[R]="电阻R(Ω)";//参数名
	}
	enum parameterName{R,parameterCount};
	static int No;
private:
	virtual double f1(Matrix &x,Matrix &oldx,double h,double t)
	{
		return (x(1,0)-x(0,0))/parameter[R];
	}
	virtual double df10(Matrix &x,Matrix &oldx,double h,double t)
	{
		return -1.0/parameter[R];
	}
	virtual double df11(Matrix &x,Matrix &oldx,double h,double t)
	{
		return 1.0/parameter[R];
	}
};

class Capacitor : public TwoTerminalElement
{
public:
    Capacitor(double *_parameter=NULL):TwoTerminalElement("C","C",2,_parameter,parameterCount),setInitVoltage(false)
	{
		ostringstream out;
		out<<++No;
		name+=out.str();
		if(_parameter==NULL)
        {
			parameter[C]=1.0e-5;//默认值
            parameter[V0]=0.0;
        }
        parameterViewName=new const char*[parameterCount];
		parameterViewName[C]="电容C(F)";//参数名
        parameterViewName[V0]="初始电压V0(V)";
    }
    bool setInitVoltage;
    enum parameterName{C,V0,parameterCount};
	static int No;
private:
	virtual double f1(Matrix &x,Matrix &oldx,double h,double t)
	{//将电容转化为等效线性元件
        if(setInitVoltage&&t<EPS)
            return (x(1,0)-x(0,0)-parameter[V0])*parameter[C]/h;
        else
            return (x(1,0)+oldx(0,0)-x(0,0)-oldx(1,0))*parameter[C]/h;
	}
	virtual double df10(Matrix &x,Matrix &oldx,double h,double t)
	{
		return -parameter[C]/h;
	}
	virtual double df11(Matrix &x,Matrix &oldx,double h,double t)
	{
		return parameter[C]/h;
	}
};

class Inductor : public TwoTerminalElement
{
public:
    Inductor(double *_parameter=NULL):TwoTerminalElement("L","L",1,_parameter,parameterCount),setInitCurrent(false)
	{
		ostringstream out;
		out<<++No;
		name+=out.str();
		if(_parameter==NULL)
        {
			parameter[L]=1.0e-5;//默认值
            parameter[I0]=0.0;
        }
        parameterViewName=new const char*[parameterCount];
		parameterViewName[L]="电感L(H)";//参数名
        parameterViewName[I0]="初始电流I0(A)";
	}
    enum parameterName{L,I0,parameterCount};
    bool setInitCurrent;
	static int No;
private:
	virtual double f1(Matrix &x,Matrix &oldx,double h,double t)
	{//将电感转化为等效线性元件
        if(setInitCurrent&&t==0.0)
            return (x(0,0)+(x(1,0)-parameter[I0])*parameter[L]/h);
        else
            return (x(0,0)+(x(1,0)-oldx(1,0))*parameter[L]/h);
	}
	virtual double df10(Matrix &x,Matrix &oldx,double h,double t)
	{
		return 1.0;
	}
	virtual double df11(Matrix &x,Matrix &oldx,double h,double t)
	{
		return parameter[L]/h;
	}
};

class Diode : public TwoTerminalElement
{
public:
	Diode(double *_parameter=NULL):TwoTerminalElement("D","D",2,_parameter,parameterCount)
	{
		ostringstream out;
		out<<++No;
		name+=out.str();
		if(_parameter==NULL)
		{
			parameter[Is]=1.0e-14;//默认值
			parameter[N]=1.0;
			parameter[T]=27.0;
		}
        parameterViewName=new const char*[parameterCount];
		parameterViewName[Is]="饱和电流Is(A)";//参数名
		parameterViewName[N]="发射系数N";
		parameterViewName[T]="温度T(°C)";
	}
	enum parameterName{Is,N,T,parameterCount};
	static int No;
private:
	virtual double f1(Matrix &x,Matrix &oldx,double h,double t)
	{
		return (-parameter[Is]*(exp((x(0,0)-x(1,0))*E/(parameter[N]*KB*(TK+parameter[T]))-1.0)));
	}
	virtual double df10(Matrix &x,Matrix &oldx,double h,double t)
	{
		return (-parameter[Is]*exp((x(0,0)-x(1,0))*E/(parameter[N]*KB*(TK+parameter[T])))*E/(parameter[N]*KB*(TK+parameter[T])));
	}
	virtual double df11(Matrix &x,Matrix &oldx,double h,double t)
	{
		return (parameter[Is]*exp((x(0,0)-x(1,0))*E/(parameter[N]*KB*(TK+parameter[T])))*E/(parameter[N]*KB*(TK+parameter[T])));
	}
};

class VoltageSource : public TwoTerminalElement
{
public:
	VoltageSource(int _parameterNum,string _type="V",double *_parameter=NULL):TwoTerminalElement("V",_type,1,_parameter,_parameterNum)
	{
		ostringstream out;
		out<<++No;
		name+=out.str();
	}
	static int No;
private:
	virtual double v(double t)=0;//电压随时间变化的表达式
	virtual double f1(Matrix &x,Matrix &oldx,double h,double t)
	{
		return (x(0,0)+v(t));
	}
	virtual double df10(Matrix &x,Matrix &oldx,double h,double t)
	{
		return 1.0;
	}
	virtual double df11(Matrix &x,Matrix &oldx,double h,double t)
	{
		return 0.0;
	}
};

class CurrentSource : public TwoTerminalElement
{
public:
	CurrentSource(int _parameterNum,string _type="I",double *_parameter=NULL):TwoTerminalElement("I",_type,2,_parameter,_parameterNum)
	{
		ostringstream out;
		out<<++No;
		name+=out.str();
	}
	static int No;
private:
	virtual double i(double t)=0;//电流随时间变化的表达式
	virtual double f1(Matrix &x,Matrix &oldx,double h,double t)
	{
		return i(t);
	}
	virtual double df10(Matrix &x,Matrix &oldx,double h,double t)
	{
        return 0.0;
	}
	virtual double df11(Matrix &x,Matrix &oldx,double h,double t)
	{
		return 0.0;
	}
};

class VDC : public VoltageSource
{
public:
	VDC(double *_parameter=NULL):VoltageSource(parameterCount,"VDC",_parameter)
	{
		if(_parameter==NULL)
			parameter[V0]=0.0;//默认值
        parameterViewName=new const char*[parameterCount];
		parameterViewName[V0]="电压V0(V)";//参数名
	}
	enum parameterName{V0,parameterCount};
private:
	virtual double v(double t)
	{
		return parameter[V0];
	}
};

class IDC : public CurrentSource
{
public:
	IDC(double *_parameter=NULL):CurrentSource(parameterCount,"IDC",_parameter)
	{
		if(_parameter==NULL)
			parameter[I0]=0.0;//默认值
        parameterViewName=new const char*[parameterCount];
		parameterViewName[I0]="电流I0(V)";//参数名
	}
	enum parameterName{I0,parameterCount};
private:
	virtual double i(double t)
	{
		return parameter[I0];
	}
};

class VPulse : public VoltageSource
{
public:
	VPulse(double *_parameter=NULL):VoltageSource(parameterCount,"VPulse",_parameter)
	{
		if(_parameter==NULL)
		{
			parameter[V1]=5.0;//默认值
			parameter[V2]=-5.0;
			parameter[TD]=0.0;
			parameter[TR]=0.0;
			parameter[TF]=0.0;
			parameter[PW]=0.5e-3;
			parameter[PER]=1.0e-3;
		}
        parameterViewName=new const char*[parameterCount];
		parameterViewName[V1]="低电平V1(V)";//参数名
		parameterViewName[V2]="高电平V2(V)";
		parameterViewName[TD]="初始延迟时间TD(s)";
		parameterViewName[TR]="脉冲上升时间TR(s)";
		parameterViewName[TF]="脉冲下降时间TF(s)";
		parameterViewName[PW]="脉冲宽度PW(s)";
		parameterViewName[PER]="信号周期PER(s)";
	}
	enum parameterName{V1,V2,TD,TR,TF,PW,PER,parameterCount};
private:
	virtual double v(double t)
	{
		if(t<parameter[TD])
			return parameter[V1];
		else
		{
			double modTime=fmod(t-parameter[TD],parameter[PER]);
			if(modTime<parameter[TR])
				return (parameter[V1]+(parameter[V2]-parameter[V1])*modTime/parameter[TR]);
			else if(modTime<parameter[TR]+parameter[PW])
				return (parameter[V2]);
			else if(modTime<parameter[TR]+parameter[PW]+parameter[TF])
				return (parameter[V2]+(modTime-parameter[TR]-parameter[PW])*(parameter[V1]-parameter[V2])/parameter[TF]);
			else
				return (parameter[V1]);
		}
	}
};

class IPulse : public CurrentSource
{
public:
    IPulse(double *_parameter=NULL):CurrentSource(parameterCount,"IPulse",_parameter)
    {
        if(_parameter==NULL)
        {
            parameter[I1]=5.0;//默认值
            parameter[I2]=-5.0;
            parameter[TD]=0.0;
            parameter[TR]=0.0;
            parameter[TF]=0.0;
            parameter[PW]=0.5e-3;
            parameter[PER]=1.0e-3;
        }
        parameterViewName=new const char*[parameterCount];
        parameterViewName[I1]="低I1(A)";//参数名
        parameterViewName[I2]="高I2(A)";
        parameterViewName[TD]="初始延迟时间TD(s)";
        parameterViewName[TR]="脉冲上升时间TR(s)";
        parameterViewName[TF]="脉冲下降时间TF(s)";
        parameterViewName[PW]="脉冲宽度PW(s)";
        parameterViewName[PER]="信号周期PER(s)";
    }
    enum parameterName{I1,I2,TD,TR,TF,PW,PER,parameterCount};
private:
    virtual double i(double t)
    {
        if(t<parameter[TD])
            return parameter[I1];
        else
        {
            double modTime=fmod(t-parameter[TD],parameter[PER]);
            if(modTime<parameter[TR])
                return (parameter[I1]+(parameter[I2]-parameter[I1])*modTime/parameter[TR]);
            else if(modTime<parameter[TR]+parameter[PW])
                return (parameter[I2]);
            else if(modTime<parameter[TR]+parameter[PW]+parameter[TF])
                return (parameter[I2]+(modTime-parameter[TR]-parameter[PW])*(parameter[I1]-parameter[I2])/parameter[TF]);
            else
                return (parameter[I1]);
        }
    }
};

class VSin : public VoltageSource
{
public:
	VSin(double *_parameter=NULL):VoltageSource(parameterCount,"VSin",_parameter)
	{
		if(_parameter==NULL)
		{
			parameter[V0]=0.0;//默认值
			parameter[Vp]=0.0;
			parameter[frequency]=1000.0;
			parameter[phase]=0.0;
		}
        parameterViewName=new const char*[parameterCount];
		parameterViewName[V0]="直流分量V0(V)";//参数名
		parameterViewName[Vp]="峰值电压Vp(V)";
		parameterViewName[frequency]="频率f(Hz)";
		parameterViewName[phase]="初始相位φ(deg)";
	}
	enum parameterName{V0,Vp,frequency,phase,parameterCount};
private:
	virtual double v(double t)
	{
		return (parameter[V0]+parameter[Vp]*sin(2*PI*parameter[frequency]*t+parameter[phase]*PI/180.0));
	}
};

class VAC : public VoltageSource
{
public:
	VAC(double *_parameter=NULL):VoltageSource(parameterCount,"VAC",_parameter),frequency(1000.0)
	{
		if(_parameter==NULL)
		{
			parameter[V0]=0.0;//默认值
			parameter[Vp]=1.0;
		}
        parameterViewName=new const char*[parameterCount];
		parameterViewName[V0]="直流分量V0(V)";//参数名
		parameterViewName[Vp]="峰值电压Vp(V)";
	}
	enum parameterName{V0,Vp,parameterCount};
	double frequency;
private:
	virtual double v(double t)
	{
		return (parameter[V0]+parameter[Vp]*sin(2*PI*frequency*t));
	}
};

class Transistor : public ThreeTerminalElement
{
public:
	Transistor(int _parameterNum,string _type="T",double *_parameter=NULL):ThreeTerminalElement("T",_type,3,_parameter,_parameterNum)
	{
		ostringstream out;
		out<<++No;
		name+=out.str();
	}
	static int No;
};

class BJT : public Transistor
{
public:
	BJT(double *_parameter=NULL,string _type="BJT"):Transistor(parameterCount,_type,_parameter)
	{
		if(_parameter==NULL)
		{
			parameter[Is]=1.0e-14;//默认值
			parameter[BF]=300.0;
			parameter[BR]=1.0;
			parameter[VAF]=200.0;
			parameter[VAR]=200.0;
			parameter[T]=27.0;
		}
        parameterViewName=new const char*[parameterCount];
		parameterViewName[Is]="饱和电流Is(A)";//参数名
		parameterViewName[BF]="正向电流增益βF";
		parameterViewName[BR]="反向电流增益βR";
		parameterViewName[VAF]="正向厄利电压VAF(V)";
		parameterViewName[VAR]="反向厄利电压VAR(V)";
		parameterViewName[T]="温度T(°C)";
	}
	enum parameterName{Is,BF,BR,VAF,VAR,T,parameterCount};
};

class NPN : public BJT
{
public:
	NPN(double *_parameter=NULL):BJT(_parameter,"NPN"){}
private:
	virtual double f1(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double ICC=parameter[Is]*(exp((x(1,0)-x(2,0))/VT)-1.0)*(1.0+(x(0,0)-x(2,0))/parameter[VAF]);
		const double IEC=parameter[Is]*(exp((x(1,0)-x(0,0))/VT)-1.0)*(1.0-(x(0,0)-x(2,0))/parameter[VAR]);
		return (ICC/parameter[BF]+IEC/parameter[BR]+Gmin*((x(1,0)-x(2,0))/parameter[BF]+(x(1,0)-x(0,0))/parameter[BR]));
	}
	virtual double f2(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double ICC=parameter[Is]*(exp((x(1,0)-x(2,0))/VT)-1.0)*(1.0+(x(0,0)-x(2,0))/parameter[VAF]);
		const double IEC=parameter[Is]*(exp((x(1,0)-x(0,0))/VT)-1.0)*(1.0-(x(0,0)-x(2,0))/parameter[VAR]);
		return (IEC-ICC*(parameter[BF]+1.0)/parameter[BF]+Gmin*((x(1,0)-x(0,0))-(1.0+1.0/parameter[BF])*(x(1,0)-x(2,0))));
	}
	virtual double df10(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double dICC=parameter[Is]*(exp((x(1,0)-x(2,0))/VT)-1.0)/parameter[VAF];
		const double dIEC=-parameter[Is]*(exp((x(1,0)-x(0,0))/VT)*(parameter[VAR]+VT+x(2,0)-x(0,0))/VT-1.0)/parameter[VAR];
		return (dICC/parameter[BF]+dIEC/parameter[BR]-Gmin/parameter[BR]);
	}
	virtual double df11(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double dICC=parameter[Is]*exp((x(1,0)-x(2,0))/VT)*(1+(x(0,0)-x(2,0))/parameter[VAF])/VT;
		const double dIEC=parameter[Is]*exp((x(1,0)-x(0,0))/VT)*(1-(x(0,0)-x(2,0))/parameter[VAR])/VT;
		return (dICC/parameter[BF]+dIEC/parameter[BR]+Gmin*(1.0/parameter[BF]+1.0/parameter[BR]));
	}
	virtual double df12(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double dICC=parameter[Is]*(1.0-exp((x(1,0)-x(2,0))/VT)*(parameter[VAF]+VT+x(0,0)-x(2,0))/VT)/parameter[VAF];
		const double dIEC=parameter[Is]*(exp((x(1,0)-x(0,0))/VT)-1.0)/parameter[VAR];
		return (dICC/parameter[BF]+dIEC/parameter[BR]-Gmin/parameter[BF]);
	}
	virtual double df20(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double dICC=parameter[Is]*(exp((x(1,0)-x(2,0))/VT)-1.0)/parameter[VAF];
		const double dIEC=parameter[Is]*(exp((x(1,0)-x(0,0))/VT)*(-parameter[VAR]+VT+x(2,0)-x(0,0))/VT-1.0)/parameter[VAR];
		return (dIEC-dICC*(parameter[BF]+1.0)/parameter[BF]-Gmin);
	}
	virtual double df21(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double dICC=parameter[Is]*exp((x(1,0)-x(2,0))/VT)*(1+(x(0,0)-x(2,0))/parameter[VAF])/VT;
		const double dIEC=parameter[Is]*exp((x(1,0)-x(0,0))/VT)*(1-(x(0,0)-x(2,0))/parameter[VAR])/VT;
		return (dIEC-dICC*(parameter[BF]+1.0)/parameter[BF]-Gmin/parameter[BF]);
	}
	virtual double df22(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double dICC=parameter[Is]*(1.0-exp((x(1,0)-x(2,0))/VT)*(parameter[VAF]+VT+x(0,0)-x(2,0))/VT)/parameter[VAF];
		const double dIEC=parameter[Is]*(exp((x(1,0)-x(0,0))/VT)-1.0)/parameter[VAR];
		return (dIEC-dICC*(parameter[BF]+1.0)/parameter[BF]+Gmin*(1.0+1.0/parameter[BF]));	
	}
};

class PNP : public BJT
{
public:
	PNP(double *_parameter=NULL):BJT(_parameter,"PNP"){}
private:
	virtual double f1(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double ICC=parameter[Is]*(exp(((-x(1,0))-(-x(2,0)))/VT)-1.0)*(1.0+((-x(0,0))-(-x(2,0)))/parameter[VAF]);
		const double IEC=parameter[Is]*(exp(((-x(1,0))-(-x(0,0)))/VT)-1.0)*(1.0-((-x(0,0))-(-x(2,0)))/parameter[VAR]);
		return -(ICC/parameter[BF]+IEC/parameter[BR]+Gmin*(((-x(1,0))-(-x(2,0)))/parameter[BF]+((-x(1,0))-(-x(0,0)))/parameter[BR]));
	}
	virtual double f2(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double ICC=parameter[Is]*(exp(((-x(1,0))-(-x(2,0)))/VT)-1.0)*(1.0+((-x(0,0))-(-x(2,0)))/parameter[VAF]);
		const double IEC=parameter[Is]*(exp(((-x(1,0))-(-x(0,0)))/VT)-1.0)*(1.0-((-x(0,0))-(-x(2,0)))/parameter[VAR]);
		return -(IEC-ICC*(parameter[BF]+1.0)/parameter[BF]+Gmin*(((-x(1,0))-(-x(0,0)))-(1.0+1.0/parameter[BF])*((-x(1,0))-(-x(2,0)))));
	}
	virtual double df10(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double dICC=-parameter[Is]*(exp(((-x(1,0))-(-x(2,0)))/VT)-1.0)/parameter[VAF];
		const double dIEC=parameter[Is]*(exp(((-x(1,0))-(-x(0,0)))/VT)*(parameter[VAR]+VT+(-x(2,0))-(-x(0,0)))/VT-1.0)/parameter[VAR];
		return -(dICC/parameter[BF]+dIEC/parameter[BR]+Gmin/parameter[BR]);
	}
	virtual double df11(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double dICC=-parameter[Is]*exp(((-x(1,0))-(-x(2,0)))/VT)*(1+((-x(0,0))-(-x(2,0)))/parameter[VAF])/VT;
		const double dIEC=-parameter[Is]*exp(((-x(1,0))-(-x(0,0)))/VT)*(1-((-x(0,0))-(-x(2,0)))/parameter[VAR])/VT;
		return -(dICC/parameter[BF]+dIEC/parameter[BR]-Gmin*(1.0/parameter[BF]+1.0/parameter[BR]));
	}
	virtual double df12(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double dICC=-parameter[Is]*(1.0-exp(((-x(1,0))-(-x(2,0)))/VT)*(parameter[VAF]+VT+(-x(0,0))-(-x(2,0)))/VT)/parameter[VAF];
		const double dIEC=-parameter[Is]*(exp(((-x(1,0))-(-x(0,0)))/VT)-1.0)/parameter[VAR];
		return -(dICC/parameter[BF]+dIEC/parameter[BR]+Gmin/parameter[BF]);
	}
	virtual double df20(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double dICC=-parameter[Is]*(exp(((-x(1,0))-(-x(2,0)))/VT)-1.0)/parameter[VAF];
		const double dIEC=-parameter[Is]*(exp(((-x(1,0))-(-x(0,0)))/VT)*(-parameter[VAR]+VT+(-x(2,0))-(-x(0,0)))/VT-1.0)/parameter[VAR];
		return -(dIEC-dICC*(parameter[BF]+1.0)/parameter[BF]+Gmin);
	}
	virtual double df21(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double dICC=-parameter[Is]*exp(((-x(1,0))-(-x(2,0)))/VT)*(1+((-x(0,0))-(-x(2,0)))/parameter[VAF])/VT;
		const double dIEC=-parameter[Is]*exp(((-x(1,0))-(-x(0,0)))/VT)*(1-((-x(0,0))-(-x(2,0)))/parameter[VAR])/VT;
		return -(dIEC-dICC*(parameter[BF]+1.0)/parameter[BF]+Gmin/parameter[BF]);
	}
	virtual double df22(Matrix &x,Matrix &oldx,double h,double t)
	{
		const double VT=KB*(TK+parameter[T])/E;
		const double dICC=-parameter[Is]*(1.0-exp(((-x(1,0))-(-x(2,0)))/VT)*(parameter[VAF]+VT+(-x(0,0))-(-x(2,0)))/VT)/parameter[VAF];
		const double dIEC=-parameter[Is]*(exp(((-x(1,0))-(-x(0,0)))/VT)-1.0)/parameter[VAR];
		return -(dIEC-dICC*(parameter[BF]+1.0)/parameter[BF]-Gmin*(1.0+1.0/parameter[BF]));
	}
};

class MOSFET : public Transistor
{
public:
	MOSFET(double *_parameter=NULL,string _type="MOSFET"):Transistor(parameterCount,_type,_parameter)
	{
		if(_parameter==NULL)
		{
			parameter[kp]=100.0e-6;//默认值
			parameter[WL]=50.0;
			parameter[VTH]=0.8;
			parameter[L]=0.02;
		}
        parameterViewName=new const char*[parameterCount];
		parameterViewName[kp]="跨导系数kp(A/V^2)";//参数名
		parameterViewName[WL]="宽长比W/L";
		parameterViewName[VTH]="阈值电压VTH(V)";
		parameterViewName[L]="沟道长度调制系数λ(V^-1)";
	}
	enum parameterName{kp,WL,VTH,L,parameterCount};
};

class NMOSFET : public MOSFET
{
public:
	NMOSFET(double *_parameter=NULL):MOSFET(_parameter,"NMOSFET"){}
private:
	virtual double f1(Matrix &x,Matrix &oldx,double h,double t)
	{
		return 0.0;
	}
	virtual double f2(Matrix &x,Matrix &oldx,double h,double t)
	{
		if(x(1,0)-x(2,0)<parameter[VTH])
			return 0.0;
		else
		{
			if(x(1,0)-x(0,0)<parameter[VTH])
				return -(0.5*parameter[kp]*parameter[WL]*pow(x(1,0)-x(2,0)-parameter[VTH],2)*(1+parameter[L]*(x(0,0)-x(2,0))));
			else
				return -(parameter[kp]*parameter[WL]*((x(1,0)-x(2,0)-parameter[VTH])*(x(0,0)-x(2,0))-0.5*pow(x(0,0)-x(2,0),2))*(1+parameter[L]*(x(0,0)-x(2,0))));
		}
	}
	virtual double df10(Matrix &x,Matrix &oldx,double h,double t)
	{
		return 0.0;
	}
	virtual double df11(Matrix &x,Matrix &oldx,double h,double t)
	{
		return 0.0;
	}
	virtual double df12(Matrix &x,Matrix &oldx,double h,double t)
	{
		return 0.0;
	}
	virtual double df20(Matrix &x,Matrix &oldx,double h,double t)
	{
		if(x(1,0)-x(2,0)<parameter[VTH])
			return 0.0;
		else
		{
			if(x(1,0)-x(0,0)<parameter[VTH])
				return -(0.5*parameter[kp]*parameter[WL]*parameter[L]*pow(x(1,0)-x(2,0)-parameter[VTH],2));
			else
				return -(parameter[kp]*parameter[WL]*((x(1,0)-x(0,0)-parameter[VTH])*(1+parameter[L]*(x(0,0)-x(2,0)))+parameter[L]*((x(1,0)-x(2,0)-parameter[VTH])*(x(0,0)-x(2,0))-0.5*pow(x(0,0)-x(2,0),2))));
		}
	}
	virtual double df21(Matrix &x,Matrix &oldx,double h,double t)
	{
		if(x(1,0)-x(2,0)<parameter[VTH])
			return 0.0;
		else
		{
			if(x(1,0)-x(0,0)<parameter[VTH])
				return -(parameter[kp]*parameter[WL]*(x(1,0)-x(2,0)-parameter[VTH])*(1+parameter[L]*(x(0,0)-x(2,0))));
			else
				return -(parameter[kp]*parameter[WL]*(x(0,0)-x(2,0))*(1+parameter[L]*(x(0,0)-x(2,0))));
		}
	}
	virtual double df22(Matrix &x,Matrix &oldx,double h,double t)
	{
		if(x(1,0)-x(2,0)<parameter[VTH])
			return 0.0;
		else
		{
			if(x(1,0)-x(0,0)<parameter[VTH])
				return -(0.5*parameter[kp]*parameter[WL]*(x(1,0)-x(2,0)-parameter[VTH])*(parameter[L]*(parameter[VTH]+3*x(2,0)-x(1,0)-2*x(0,0))-2));
			else
				return -(parameter[kp]*parameter[WL]*((-x(1,0)+x(2,0)+parameter[VTH])*(1+2*parameter[L]*(x(0,0)-x(2,0)))+0.5*parameter[L]*pow(x(0,0)-x(2,0),2)));
		}
	}
};

class PMOSFET : public MOSFET
{
public:
	PMOSFET(double *_parameter=NULL):MOSFET(_parameter,"PMOSFET"){}
private:
	virtual double f1(Matrix &x,Matrix &oldx,double h,double t)
	{
		return 0.0;
	}
	virtual double f2(Matrix &x,Matrix &oldx,double h,double t)
	{
		if((-x(1,0))-(-x(2,0))<parameter[VTH])
			return 0.0;
		else
		{
			if((-x(1,0))-(-x(0,0))<parameter[VTH])
				return (0.5*parameter[kp]*parameter[WL]*pow((-x(1,0))-(-x(2,0))-parameter[VTH],2)*(1+parameter[L]*((-x(0,0))-(-x(2,0)))));
			else
				return (parameter[kp]*parameter[WL]*(((-x(1,0))-(-x(2,0))-parameter[VTH])*((-x(0,0))-(-x(2,0)))-0.5*pow((-x(0,0))-(-x(2,0)),2))*(1+parameter[L]*((-x(0,0))-(-x(2,0)))));
		}
	}
	virtual double df10(Matrix &x,Matrix &oldx,double h,double t)
	{
		return 0.0;
	}
	virtual double df11(Matrix &x,Matrix &oldx,double h,double t)
	{
		return 0.0;
	}
	virtual double df12(Matrix &x,Matrix &oldx,double h,double t)
	{
		return 0.0;
	}
	virtual double df20(Matrix &x,Matrix &oldx,double h,double t)
	{
		if((-x(1,0))-(-x(2,0))<parameter[VTH])
			return 0.0;
		else
		{
			if((-x(1,0))-(-x(0,0))<parameter[VTH])
				return -(0.5*parameter[kp]*parameter[WL]*parameter[L]*pow((-x(1,0))-(-x(2,0))-parameter[VTH],2));
			else
				return -(parameter[kp]*parameter[WL]*(((-x(1,0))-(-x(0,0))-parameter[VTH])*(1+parameter[L]*((-x(0,0))-(-x(2,0))))+parameter[L]*(((-x(1,0))-(-x(2,0))-parameter[VTH])*((-x(0,0))-(-x(2,0)))-0.5*pow((-x(0,0))-(-x(2,0)),2))));
		}
	}
	virtual double df21(Matrix &x,Matrix &oldx,double h,double t)
	{
		if((-x(1,0))-(-x(2,0))<parameter[VTH])
			return 0.0;
		else
		{
			if((-x(1,0))-(-x(0,0))<parameter[VTH])
				return -(parameter[kp]*parameter[WL]*((-x(1,0))-(-x(2,0))-parameter[VTH])*(1+parameter[L]*((-x(0,0))-(-x(2,0)))));
			else
				return -(parameter[kp]*parameter[WL]*((-x(0,0))-(-x(2,0)))*(1+parameter[L]*((-x(0,0))-(-x(2,0)))));
		}
	}
	virtual double df22(Matrix &x,Matrix &oldx,double h,double t)
	{
		if((-x(1,0))-(-x(2,0))<parameter[VTH])
			return 0.0;
		else
		{
			if((-x(1,0))-(-x(0,0))<parameter[VTH])
				return -(0.5*parameter[kp]*parameter[WL]*((-x(1,0))-(-x(2,0))-parameter[VTH])*(parameter[L]*(parameter[VTH]+3*(-x(2,0))-(-x(1,0))-2*(-x(0,0)))-2));
			else
				return -(parameter[kp]*parameter[WL]*((-(-x(1,0))+(-x(2,0))+parameter[VTH])*(1+2*parameter[L]*((-x(0,0))-(-x(2,0))))+0.5*parameter[L]*pow((-x(0,0))-(-x(2,0)),2)));
		}
	}
};

class VCVS : public FourTerminalElement
{
public:
    VCVS(double *_parameter=NULL):FourTerminalElement("E","E",3,_parameter,parameterCount)
    {
        ostringstream out;
        out<<++No;
        name+=out.str();
        if(_parameter==NULL)
        {
            parameter[Av]=1.0;//默认值
        }
        parameterViewName=new const char*[parameterCount];
        parameterViewName[Av]="电压增益Av";
    }
    enum parameterName{Av,parameterCount};
    static int No;
private:
    virtual double f1(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double f2(Matrix &x,Matrix &oldx,double h,double t) { return -x(3,0); }
    virtual double f3(Matrix &x,Matrix &oldx,double h,double t) { return (x(2,0)+parameter[Av]*(x(0,0)-x(1,0))); }
    virtual double df10(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df11(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df12(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df13(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df20(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df21(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df22(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df23(Matrix &x,Matrix &oldx,double h,double t) { return -1.0; }
    virtual double df30(Matrix &x,Matrix &oldx,double h,double t) { return parameter[Av]; }
    virtual double df31(Matrix &x,Matrix &oldx,double h,double t) { return -parameter[Av]; }
    virtual double df32(Matrix &x,Matrix &oldx,double h,double t) { return 1.0; }
    virtual double df33(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
};

class CCCS : public FourTerminalElement
{
public:
    CCCS(double *_parameter=NULL):FourTerminalElement("F","F",3,_parameter,parameterCount)
    {
        ostringstream out;
        out<<++No;
        name+=out.str();
        if(_parameter==NULL)
        {
            parameter[Ai]=1.0;//默认值
        }
        parameterViewName=new const char*[parameterCount];
        parameterViewName[Ai]="电流增益Ai";
    }
    enum parameterName{Ai,parameterCount};
    static int No;
private:
    virtual double f1(Matrix &x,Matrix &oldx,double h,double t) { return parameter[Ai]*x(3,0); }
    virtual double f2(Matrix &x,Matrix &oldx,double h,double t) { return -x(3,0); }
    virtual double f3(Matrix &x,Matrix &oldx,double h,double t) { return x(2,0); }
    virtual double df10(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df11(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df12(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df13(Matrix &x,Matrix &oldx,double h,double t) { return parameter[Ai]; }
    virtual double df20(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df21(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df22(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df23(Matrix &x,Matrix &oldx,double h,double t) { return -1.0; }
    virtual double df30(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df31(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df32(Matrix &x,Matrix &oldx,double h,double t) { return 1.0; }
    virtual double df33(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
};

class VCCS : public FourTerminalElement
{
public:
    VCCS(double *_parameter=NULL):FourTerminalElement("G","G",4,_parameter,parameterCount)
    {
        ostringstream out;
        out<<++No;
        name+=out.str();
        if(_parameter==NULL)
        {
            parameter[Gm]=0.001;//默认值
        }
        parameterViewName=new const char*[parameterCount];
        parameterViewName[Gm]="跨导增益Gm(S)";
    }
    enum parameterName{Gm,parameterCount};
    static int No;
private:
    virtual double f1(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double f2(Matrix &x,Matrix &oldx,double h,double t) { return -parameter[Gm]*(x(0,0)-x(1,0)); }
    virtual double f3(Matrix &x,Matrix &oldx,double h,double t) { return parameter[Gm]*(x(0,0)-x(1,0)); }
    virtual double df10(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df11(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df12(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df13(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df20(Matrix &x,Matrix &oldx,double h,double t) { return -parameter[Gm]; }
    virtual double df21(Matrix &x,Matrix &oldx,double h,double t) { return parameter[Gm]; }
    virtual double df22(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df23(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df30(Matrix &x,Matrix &oldx,double h,double t) { return parameter[Gm]; }
    virtual double df31(Matrix &x,Matrix &oldx,double h,double t) { return -parameter[Gm]; }
    virtual double df32(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df33(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
};

class CCVS : public FourTerminalElement
{
public:
    CCVS(double *_parameter=NULL):FourTerminalElement("H","H",2,_parameter,parameterCount)
    {
        ostringstream out;
        out<<++No;
        name+=out.str();
        if(_parameter==NULL)
        {
            parameter[Rm]=1000.0;//默认值
        }
        parameterViewName=new const char*[parameterCount];
        parameterViewName[Rm]="跨阻增益Rm(Ω)";
    }
    enum parameterName{Rm,parameterCount};
    static int No;
private:
    virtual double f1(Matrix &x,Matrix &oldx,double h,double t) { return -x(2,0); }
    virtual double f2(Matrix &x,Matrix &oldx,double h,double t) { return x(1,0); }
    virtual double f3(Matrix &x,Matrix &oldx,double h,double t) { return x(0,0)+parameter[Rm]*x(2,0); }
    virtual double df10(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df11(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df12(Matrix &x,Matrix &oldx,double h,double t) { return -1.0; }
    virtual double df13(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df20(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df21(Matrix &x,Matrix &oldx,double h,double t) { return 1.0; }
    virtual double df22(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df23(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df30(Matrix &x,Matrix &oldx,double h,double t) { return 1.0; }
    virtual double df31(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
    virtual double df32(Matrix &x,Matrix &oldx,double h,double t) { return parameter[Rm]; }
    virtual double df33(Matrix &x,Matrix &oldx,double h,double t) { return 0.0; }
};
#endif //ELEMENT_H
