#ifndef MATRIX_H
#define MATRIX_H
#include <assert.h>
#include <stddef.h>

template <class T>
inline T abs(T x) {return (x>0.0?x:(-x));}
template <class T>
inline T sign(T x) {return (x>=0.0?1.0:(-1.0));}

const double EPS=1.0e-16;

class Matrix
{
public:
	int rowCount() const {return row;}
	int columnCount() const {return column;}
	double &operator()(int r,int c)
	{
		assert(r>=0&&r<row&&c>=0&&c<column);
		return data[column*r+c];
	}
	Matrix operator-();
    Matrix &operator=(Matrix m);
    friend Matrix operator+(Matrix m1,Matrix m2);
    friend Matrix operator-(Matrix m1,Matrix m2);
    friend Matrix operator*(double d,Matrix m);
    friend Matrix operator*(Matrix m1,Matrix m2);
    friend Matrix augmentedMatrix(Matrix &m1,Matrix &m2);
	Matrix transpose();
	Matrix inverse();
	Matrix FFT();
	double det();
	static Matrix identifyMatrix(int n);
	Matrix(int _row=0,int _column=0):row(_row),column(_column)
	{//构造函数
		if(_row*_column!=0)
		{
			data=new double[_row*_column];
			for (int i = 0; i < row*column; i++)
				data[i] = 0.0;//将元素初始化为0.0
		}
		else
			data=NULL;
	}
	Matrix(const Matrix &m):row(m.row),column(m.column)
	{//拷贝构造函数
		if(row*column!=0)
		{
			data=new double[row*column];
			for(int i=0;i<row*column;i++)
				data[i]=m.data[i];
		}
		else
			data=NULL;
	}
	~Matrix()
	{//析构函数
		if(data!=NULL)
			delete []data;
	}
private:
	int row;
	int column;
	double *data;
	void forwardElimination(int &permutationCount);
};

#endif // MATRIX_H
