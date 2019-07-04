#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <set>
#include "flist.h"
#include "element.h"
#include "graphicsitem.h"
#include "graphicsscene.h"

const int GRID_WIDTH=20;
const double PINVIEW_LENGTH=10.0;
const double PARTVIEW_LENGTH=80.0;
const double DELTA=0.5;
const double TEXT_HEIGHT=20.0;
const double WIRE_WIDTH_TO_SELECT=8.0;//可选择连线的矩形宽度
const double COLLIDING_CIRCLE_RADIUS=10.0;

template <class T>
inline T snapToGrid(T x) {return floor(x/GRID_WIDTH+0.5)*GRID_WIDTH;}//对齐到网格

namespace Ui {
class MainWindow;
}
class Structure;
class Calculate;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
	PartView *currentSelectPart;
	GraphicsScene *sceneOfSchematic;
	void setPartParameter();
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
	
private slots:
	void drawWire();
	void rotatePart();
	void searchPart(QString partType);
	void viewPartImage();
	void initPart(QListWidgetItem *part);
	void setPartParameter(int pinIndex,QString value);
	void savePartParameter(int row, int column);
	void setSelectedPartParameter();
    void setEditState(int row, int column);
	bool saveNetlist();
	bool saveSchematic();
	void newSchematic();
	void openSchematic();
	void runSimulation();
	bool copy();
	void paste();
	void cut();
	void selectAll();
	void deleteItem();
	void help();

private:
	bool isSaved;
	QString fileName;
	QString sCurrentSelectPart;
	void keyPressEvent(QKeyEvent *event);
	void closeEvent(QCloseEvent *event);
	bool saveOnCloseQuestion();
	void showSimulationText(double h,Calculate *calculate,Structure *structure);
	void findConnectedPart(Element *currentPart,set<Element *> &isConnected);
	void outputItems(ostream &output,QList<QGraphicsItem *> items);
	QList<QGraphicsItem *> inputItems(istream &input,bool autoNo);
	PartView *newPartView(Element *part);
	GraphicsScene *newGraphicsScene();
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
