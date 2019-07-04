#include "element.h"
#include "mainwindow.h"
#include <QPixmap>
#include <QString>
#include <QFont>

map<string,string> Element::viewNameToType;
double Element::Gmin=1.0e-12;
int Ground::No=0;
int Resistor::No=0;
int Capacitor::No=0;
int Inductor::No=0;
int Diode::No=0;
int VoltageSource::No=0;
int CurrentSource::No=0;
int Transistor::No=0;
int VCVS::No=0;
int CCCS::No=0;
int VCCS::No=0;
int CCVS::No=0;

double etof(string s)
{//工程计数法表示转换为浮点数
	double tmp=atof(s.c_str());
	int eng=0;
	for(eng=0;((s[eng]<='9'&&s[eng]>='0')||s[eng]=='.'||s[eng]=='-')&&eng<s.size();eng++);
	switch(s[eng])
	{
	case 'e':case 'E':tmp*=pow((double)10.0,atoi(s.c_str()+eng+1));break;
	case 'f':case 'F':tmp*=1.0e-15;break;
	case 'p':case 'P':tmp*=1.0e-12;break;
	case 'n':case 'N':tmp*=1.0e-9;break;
	case 'u':case 'U':tmp*=1.0e-6;break;
	case 'm':case 'M':
		if(eng+2<s.size()&&(s[eng+1]=='e'||s[eng+1]=='E')&&(s[eng+2]=='g'||s[eng+2]=='G'))
            tmp*=1.0e6;
		else
            tmp*=1.0e-3;
        break;
	case 'k':case 'K':tmp*=1.0e3;break;
	case 'g':case 'G':tmp*=1.0e9;break;
	case 't':case 'T':tmp*=1.0e12;break;
	}
	return tmp;
}

string ftoe(double d,int prec)
{//浮点数转换为工程计数法表示
	ostringstream out;
	if(prec>-1)//设置精度
        out.precision(prec);
    switch((int)floor(log10(fabs(d))/3.0))
	{	
	case -5:out<<d*1.0e15<<'f';return out.str();break;
	case -4:out<<d*1.0e12<<'p';return out.str();break;
	case -3:out<<d*1.0e9<<'n';return out.str();break;
	case -2:out<<d*1.0e6<<'u';return out.str();break;
	case -1:out<<d*1.0e3<<'m';return out.str();break;
	case 0:out<<d;return out.str();break;
	case 1:out<<d*1.0e-3<<'k';return out.str();break;
	case 2:out<<d*1.0e-6<<"Meg";return out.str();break;
	case 3:out<<d*1.0e-9<<'G';return out.str();break;
	case 4:out<<d*1.0e-12<<'T';return out.str();break;
	}
	if(abs(d)>EPS)
        out<<d*pow((double)10.0,-floor(log10(fabs(d))))<<'E'<<floor(log10(fabs(d)));
	else
		out<<0.0;
	return out.str();
}

void Element::setStaticNo(istream &input)
{//从输入流input中读取并设置各类型元件的staticNo
	input>>Ground::No>>Resistor::No>>Capacitor::No>>Inductor::No
		>>Diode::No>>VoltageSource::No>>CurrentSource::No>>Transistor::No;
}
void Element::showStaticNo(ostream &output)
{//输出各类型元件的staticNo至输出流output
	output<<Ground::No<<'\t'<<Resistor::No<<'\t'<<Capacitor::No<<'\t'<<Inductor::No<<'\t'
		<<Diode::No<<'\t'<<VoltageSource::No<<'\t'<<CurrentSource::No<<'\t'<<Transistor::No<<endl;
}
void Element::mapInit()
{//viewNameToType初始化函数
    viewNameToType["电阻(R)"]="R";
	viewNameToType["电容(C)"]="C";
	viewNameToType["电感(L)"]="L";
	viewNameToType["二极管(D)"]="D";
	viewNameToType["恒压源(VDC)"]="VDC";
	viewNameToType["恒流源(IDC)"]="IDC";
    viewNameToType["脉波电压源(VPulse)"]="VPulse";
    viewNameToType["脉波电流源(IPulse)"]="IPulse";
	viewNameToType["正弦电压源(VSin)"]="VSin";
	viewNameToType["交流电压源(VAC)"]="VAC";
	viewNameToType["NPN-BJT"]="NPN";
	viewNameToType["PNP-BJT"]="PNP";
	viewNameToType["NMOSFET"]="NMOSFET";
	viewNameToType["PMOSFET"]="PMOSFET";
    viewNameToType["地(GND)"]="GND";
    viewNameToType["压控压源(VCVS)"]="E";
    viewNameToType["流控流源(CCCS)"]="F";
    viewNameToType["压控流源(VCCS)"]="G";
    viewNameToType["流控压源(CCVS)"]="H";
}

Element *Element::newElement(string type)
{//新建元件，返回新建元件的指针
	if(type=="GND")
		return (new Ground);
	else if(type=="R")
		return (new Resistor);
	else if(type=="C")
		return (new Capacitor);
	else if(type=="L")
		return (new Inductor);
	else if(type=="D")
		return (new Diode);
	else if(type=="VDC")
		return (new VDC);
	else if(type=="IDC")
		return (new IDC);
	else if(type=="VPulse")
		return (new VPulse);
	else if(type=="VSin")
		return (new VSin);
	else if(type=="VAC")
		return (new VAC);
    else if(type=="IPulse")
        return (new IPulse);
	else if(type=="NPN")
		return (new NPN);
	else if(type=="PNP")
		return (new PNP);
	else if(type=="NMOSFET")
		return (new NMOSFET);
    else if(type=="PMOSFET")
        return (new PMOSFET);
    else if(type=="E")
        return (new VCVS);
    else if(type=="F")
        return (new CCCS);
    else if(type=="G")
        return (new VCCS);
    else if(type=="H")
        return (new CCVS);
	return NULL;
}

QPixmap Element::imagePixmap(string type)
{//返回该元件所对应在原理图上显示的图片
	return QPixmap((string(":/part/")+type).c_str());
}

QPixmap Element::viewImagePixmap(string type)
{//返回该元件所对应供预览的图片
	return QPixmap((string(":/viewPart/")+type).c_str());
}

istream &operator>> (istream &input,Element &part)
{
	//input>>part.name;
	int i;
	string tmp;
	for(i=0;i<part.pinNum;i++)
		input>>(part.pin)[i];
    if(part.type=="C")
    {
        for(i=0;i<part.parameterNum-1;i++)
        {
            input>>tmp;
            part.parameter[i]=etof(tmp);
        }
        char ctmp;
        input>>ctmp;
        input.putback(ctmp);
        if(ctmp=='+')
        {
            input>>tmp;
            if(tmp=="+V0:")
            {
                ((Capacitor &)part).setInitVoltage=true;
                input>>tmp;
                part.parameter[i]=etof(tmp);
            }
        }
    }
    else if(part.type=="L")
    {
        for(i=0;i<part.parameterNum-1;i++)
        {
            input>>tmp;
            part.parameter[i]=etof(tmp);
        }
        char ctmp;
        input>>ctmp;
        input.putback(ctmp);
        if(ctmp=='+')
        {
            input>>tmp;
            if(tmp=="+I0:")
            {
                ((Inductor &)part).setInitCurrent=true;
                input>>tmp;
                part.parameter[i]=etof(tmp);
            }
        }
    }
    else
        for(i=0;i<part.parameterNum;i++)
        {
            input>>tmp;
            part.parameter[i]=etof(tmp);
        }
	return input;
}

ostream &operator<< (ostream &output,Element &part)
{
	output<<part.name<<'\t';
	int i;
	for(i=0;i<part.pinNum;i++)
		output<<(part.pin)[i]<<'\t';
    if(part.type=="C")
    {
        for(i=0;i<part.parameterNum-1;i++)
            output<<ftoe(part.showParameter(i))<<'\t';
        if(((Capacitor &)part).setInitVoltage)
            output<<"+V0: "<<ftoe(part.showParameter(i))<<'\t';
    }
    else if(part.type=="L")
    {
        for(i=0;i<part.parameterNum-1;i++)
            output<<ftoe(part.showParameter(i))<<'\t';
        if(((Inductor &)part).setInitCurrent)
            output<<"+I0: "<<ftoe(part.showParameter(i))<<'\t';
    }
    else
        for(i=0;i<part.parameterNum;i++)
            output<<ftoe(part.showParameter(i))<<'\t';
	return output;
}
