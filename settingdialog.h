#ifndef SETTINGDIALOG
#define SETTINGDIALOG
#include <QDialog>
#include "flist.h"
#include "element.h"
#include "simulate.h"

namespace Ui {
class SimulationSetting;
}

class SimulationSetting : public QDialog
{
    Q_OBJECT

public:
    Ui::SimulationSetting *ui;
    explicit SimulationSetting(Structure *_structure,QWidget *parent = 0);
    ~SimulationSetting();

private slots:
	void setDCPrimaryStepLabel();
	void setDCSecondaryStepLabel();
	void setACStepLabel();
	void setACParametricStepLabel();
	void searchDCPart(QString partName);
	void searchDCParameter(QString parameterName);
	void searchACPart(QString partName);
	void searchACParameter(QString parameterName);
	void setDCParameterList();
	void setACParameterList();
	void enableSecondarySweep(bool enable);
	void enableParametricSweep(bool enable);
	void checkInput();

private:
	Structure *structure;
};
#endif // SETTINGDIALOG_H