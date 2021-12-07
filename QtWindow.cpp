#include "QtWindow.hpp"
#include "./ui_QtWindow.h"
#include "SdlRenderer.hpp"

void QtWindow::Construct(QWidget *parent)
{
	ui = new Ui::QtWindow();
	ui->setupUi(this);
}

QtWindow::QtWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(nullptr),
	  m_pSdlRenderer(nullptr)
{
	Construct(parent);
}

QtWindow::QtWindow(QWidget *parent, SdlRenderer *a_pSdlRenderer)
	: QMainWindow(parent)
    , ui(nullptr),
	  m_pSdlRenderer(a_pSdlRenderer)
{
	Construct(parent);
}

QtWindow::~QtWindow()
{
    delete ui;
}


void QtWindow::moveUpButton_pressed()
{
	if(m_pSdlRenderer != nullptr)
		m_pSdlRenderer->postMessage(SdlRenderer::RendererMsg(SdlRenderer::eRenderMsg_MoveUp, 10.0f));
}


void QtWindow::moveLeftButton_pressed()
{
	if(m_pSdlRenderer != nullptr)
		m_pSdlRenderer->postMessage(SdlRenderer::RendererMsg(SdlRenderer::eRenderMsg_MoveLeft, 10.0f));
}


void QtWindow::moveRightButton_pressed()
{
	if(m_pSdlRenderer != nullptr)
		m_pSdlRenderer->postMessage(SdlRenderer::RendererMsg(SdlRenderer::eRenderMsg_MoveRight, 10.0f));
}


void QtWindow::moveDownButton_pressed()
{
	if(m_pSdlRenderer != nullptr)
		m_pSdlRenderer->postMessage(SdlRenderer::RendererMsg(SdlRenderer::eRenderMsg_MoveDown, 10.0f));
}

