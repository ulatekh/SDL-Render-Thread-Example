#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#if QT_VERSION_MAJOR >= 5
	#include <QMainWindow>
#else // QT_VERSION_MAJOR >= 5
	#include <Qt/qmainwindow.h>
#endif // QT_VERSION_MAJOR >= 5

// Forward declarations.
class SdlRenderer;

QT_BEGIN_NAMESPACE
namespace Ui { class QtWindow; }
QT_END_NAMESPACE

class QtWindow : public QMainWindow
{
    Q_OBJECT

public:
    QtWindow(QWidget *parent = nullptr);
	QtWindow(QWidget *parent, SdlRenderer *a_pSdlRenderer);
    ~QtWindow();

private slots:
	void moveUpButton_pressed();
    void moveLeftButton_pressed();
    void moveRightButton_pressed();
    void moveDownButton_pressed();

private:
	void Construct(QWidget *parent);
    Ui::QtWindow *ui;
	SdlRenderer *m_pSdlRenderer;
};
#endif // MAINWINDOW_H
