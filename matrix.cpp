#include <cmath>
#include "matrix.h"
const double PI=4.0*atan(1.0);

Matrix Matrix::operator-()
{
	Matrix opposite(row,column);
	for(int i=0;i<row;i++)
		for(int j=0;j<column;j++)
			opposite(i,j)=-(*this)(i,j);
	return opposite;
}

Matrix operator+(Matrix m1,Matrix m2)
{
	assert(m1.row==m2.row&&m1.column==m2.column);
	Matrix sum(m1.row,m1.column);
	for(int i=0;i<m1.row;i++)
		for(int j=0;j<m1.column;j++)
			sum(i,j)=m1(i,j)+m2(i,j);
	return sum;
}

Matrix operator-(Matrix m1,Matrix m2)
{
	assert(m1.row==m2.row&&m1.column==m2.column);
	Matrix diff(m1.row,m1.column);
	for(int i=0;i<m1.row;i++)
		for(int j=0;j<m1.column;j++)
			diff(i,j)=m1(i,j)-m2(i,j);
	return diff;
}

Matrix &Matrix::operator=(Matrix m)
{
	if(row!=m.row||column!=m.column)
	{
		if(data!=NULL)
			delete []data;
		if(m.row*m.column!=0)
			data=new double[m.row*m.column];
		else
			data=NULL;
		row=m.row;
		column=m.column;
	}
	for(int i=0;i<m.row;i++)
		for(int j=0;j<m.column;j++)
			(*this)(i,j)=m(i,j);
	return *this;
}

Matrix operator*(double d,Matrix m)
{
	Matrix mul(m.row,m.column);
	for(int i=0;i<m.row;i++)
		for(int j=0;j<m.column;j++)
			mul(i,j)=d*m(i,j);
	return mul;
}

Matrix operator*(Matrix m1,Matrix m2)
{
	assert(m1.column==m2.row);
	Matrix mul(m1.row,m2.column);
	for(int i=0;i<m1.row;i++)
		for(int j=0;j<m2.column;j++)
			for(int k=0;k<m1.column;k++)
				mul(i,j)+=m1(i,k)*m2(k,j);
	return mul;
}

Matrix augmentedMatrix(Matrix &m1,Matrix &m2)
{
	assert(m1.row==m2.row);
	Matrix aug(m1.row,m1.row+m2.row);
	for(int i=0;i<m1.row;i++)
	{
		for(int j=0;j<m1.column;j++)
			aug(i,j)=m1(i,j);
		for(int j=0;j<m2.column;j++)
			aug(i,j+m1.column)=m2(i,j);
	}
	return aug;
}

Matrix Matrix::transpose()
{
	Matrix tran(column,row);
	for(int i=0;i<row;i++)
		for(int j=0;j<column;j++)
			tran(j,i)=(*this)(i,j);
	return tran;
}

Matrix Matrix::inverse()
{//使用高斯-约旦方法进行矩阵求逆
	assert(row==column);
    Matrix im=identifyMatrix(row);
    Matrix aug=augmentedMatrix(*this,im);
	int permutationCount=0,i,j,k;
    double mul;
	aug.forwardElimination(permutationCount);
	//assert(permutationCount!=-1);//保证矩阵可逆
	for(j=row-1;j>0;j--)
	{
		//assert(abs(aug(j,j))>EPS);//保证矩阵可逆
		for(i=j-1;i>=0;i--)//向上进行消去操作，得到一个对角矩阵
		{
			mul=aug(i,j)/aug(j,j);
			for(k=0;k<aug.column;k++)
				aug(i,k)-=aug(j,k)*mul;
		}
	}
	Matrix inv(row,column);
	for(int i=0;i<row;i++)//用主元除各对应行，以在左半部分得到一个单位阵，并保留后row列
		for(int j=0;j<column;j++)
			inv(i,j)=aug(i,j+column)/aug(i,i);
	return inv;
}

Matrix Matrix::FFT()
{
	assert(column==1||column==2);//假设列数为1或2
	int step=row,h=0,index,divCnt,i,j,k,z;
	for(;(step&1)==0;step>>=1)
		h++;
	divCnt=1<<h;
	int *rev=new int[divCnt];
	for(rev[0]=0,i=0;i<divCnt;i++)
		rev[i]=(rev[i>>1]>>1)|((i&1)<<(h-1));
	Matrix fft(row,2),w(1,2),u(1,2),t(1,2);
	for(i=0;i<divCnt;i++) 
	{
		for(z=i,index=k=rev[i]*step;z<row;z+=divCnt,index++)
		{//初始化，将数据以一定顺序重新排列，存储至fft中
			fft(index,0)=(*this)(z,0);
			if(column>1)
				fft(index,1)=(*this)(z,1);
		}
		if(step>1)
		{//将k~k+step部分数据进行DFT操作
			Matrix dft(step,2);
			for(int i=0;i<step;i++)
				for(int j=0;j<step;j++)
				{
					w(0,0)=cos(2*PI*i*j/step);w(0,1)=-sin(2*PI*i*j/step);
					t(0,0)=w(0,0)*fft(j+k,0)-w(0,1)*fft(j+k,1);
					t(0,1)=w(0,1)*fft(j+k,0)+w(0,0)*fft(j+k,1);
					dft(i,0)+=t(0,0);dft(i,1)+=t(0,1);
				}
			for(int i=0;i<step;i++)
			{
				fft(i+k,0)=dft(i,0);
				fft(i+k,1)=dft(i,1);
			}
		}
	}
	delete []rev;
	for(;step<row;step<<=1)
		for(j=0;j<row;j+=step<<1)
			for(k=j;k<j+step;k++)
			{
				w(0,0)=cos(PI*(k-j)/step);w(0,1)=-sin(PI*(k-j)/step);
				u(0,0)=fft(k,0);u(0,1)=fft(k,1);
				t(0,0)=w(0,0)*fft(k+step,0)-w(0,1)*fft(k+step,1);
				t(0,1)=w(0,1)*fft(k+step,0)+w(0,0)*fft(k+step,1);
				fft(k,0)=u(0,0)+t(0,0);fft(k,1)=u(0,1)+t(0,1);
				fft(k+step,0)=u(0,0)-t(0,0);fft(k+step,1)=u(0,1)-t(0,1);
			}
	return fft;
}

double Matrix::det()
{
	assert(row==column);
	Matrix tmp(*this);
	int permutationCount=0;
	double det=1.0;
	tmp.forwardElimination(permutationCount);
	if(permutationCount==-1)
		return 0.0;
	for(int i=0;i<row;i++)
		det*=tmp(i,i);
	if(permutationCount%2)
		return -det;
	else
		return det;
}

Matrix Matrix::identifyMatrix(int n)
{
	Matrix i(n,n);
	for(int k=0;k<n;k++)
		i(k,k)=1.0;
	return i;
}

void Matrix::forwardElimination(int &permutationCount)
{//forwardElimination函数：向下依次进行消元操作
	int i,j;
	double mul;
	for(j=0;j<row-1;j++)
	{
		for(i=j+1;i<=row-1;i++)//进行列选主元，将第j列的(*this)(j,j)以下所有元素绝对值最大者所在行交换至第j行
			if(abs((*this)(i,j))>abs((*this)(j,j)))
			{
				permutationCount++;
				double temp;
				for(int k=0;k<column;k++)
				{
					temp=(*this)(i,k);
					(*this)(i,k)=(*this)(j,k);
					(*this)(j,k)=temp;
				}
			}
		if(abs((*this)(j,j))<EPS)
		{//若行列式为0，则返回错误
			permutationCount=-1;
			return;
		}
		for(i=j+1;i<row;i++)//进行消去操作，使第j列的(*this)(j,j)以下所有元素均化为0
		{
			mul=(*this)(i,j)/(*this)(j,j);
			for(int k=0;k<column;k++)
				(*this)(i,k)-=(*this)(j,k)*mul;
		}
	}
}
