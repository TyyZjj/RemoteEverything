#include <QFileDialog>
#include <QSettings>
#include <QTextCodec>
#include <QScrollBar>
#include "RemoteEverything.h"
#include "ConfigCore.h"

RemoteEverything::RemoteEverything(QWidget *parent)
    : QWidget(parent), 
	m_iResultCount(0),
	m_iCurrentCount(0),
	m_iTotalCount(0),
	m_iAccessCount(0),
	m_iUnAccessCount(0)
{
    ui.setupUi(this);

	initWidget();
}

RemoteEverything::~RemoteEverything()
{
    for (auto &dataUI : m_lstDataUI)
    {
		dataUI.pEverythingObj->close();
    }
}

void RemoteEverything::on_searchBtn_clicked()
{
	for (auto dataUI : m_lstDataUI)
	{
		if (dataUI.pEverythingObj &&
			dataUI.pEverythingObj->isEtpConnect())
		{
			bool isRecv = dataUI.pEverythingObj->isRecv();
			if (isRecv)
			{
				showRunLog(QString::fromLocal8Bit("%1 正在接收数据中, 请稍后再试.").arg(dataUI.pcInfo.strName));
				return;
			}
		}
	}

	QString str = ui.inputEdit->text();
	if (str.isEmpty())
		return;

	m_iResultCount = 0;
	m_iCurrentCount = 0;
	m_iTotalCount = 0;
	m_iAccessCount = 0;
	m_iUnAccessCount = 0;
	ui.progressBar->setValue(0);
	for (auto dataUI : m_lstDataUI)
	{
		if (dataUI.pEverythingObj &&
			dataUI.pEverythingObj->isEtpConnect())
		{
			dataUI.pTableWdg->clear();
			bool ret = dataUI.pEverythingObj->search(str);
		}
	}
	showRunLog(QString::fromLocal8Bit("开始检索."));
}

void RemoteEverything::on_copyCurrentBtn_clicked()
{
	int index = ui.stackedWidget->currentIndex();
	if (m_lstDataUI.count() > index)
	{
		sDataUI dataUI = m_lstDataUI.value(index);
		QList<QUrl> lstUrl = dataUI.pTableWdg->getAccessUrl();

		QMimeData* mimeData = new QMimeData;
		mimeData->setUrls(lstUrl);
		QApplication::clipboard()->setMimeData(mimeData);

		showRunLog(QString::fromLocal8Bit("复制 %1 条记录成功.").arg(lstUrl.count()));
	}
}

void RemoteEverything::on_copyBtn_clicked()
{
	QList<QUrl> lstUrl;
	for (sDataUI dataUI : m_lstDataUI)
	{
		QList<QUrl> lst = dataUI.pTableWdg->getAccessUrl();
		lstUrl.append(lst);
	}
	QMimeData* mimeData = new QMimeData;
	mimeData->setUrls(lstUrl);
	QApplication::clipboard()->setMimeData(mimeData);
	showRunLog(QString::fromLocal8Bit("复制 %1 条记录成功.").arg(lstUrl.count()));
}

void RemoteEverything::closeEvent(QCloseEvent* event)
{
	saveUiConfig();
	QWidget::closeEvent(event);
}

void RemoteEverything::initWidget()
{
    sConfig config = CConfigCore::GetKernel()->GetConfig();

	ui.viewSplitter1->restoreState(config.state1);
	ui.viewSplitter2->restoreState(config.state2);
	ui.viewSplitter3->restoreState(config.state3);

	ui.progressBar->setValue(0);
	initDataUI(config);
}

void RemoteEverything::initDataUI(const sConfig& config)
{
    for (auto &pcInfo : config.lstPCInfo)
    {
		QHBoxLayout* phlayout = qobject_cast<QHBoxLayout*>(ui.listFrame->layout());
		//新建按键
		QPushButton* pBtn = new QPushButton(pcInfo.strName, ui.listFrame);
        pBtn->setFixedSize(80, 30);
		phlayout->addWidget(pBtn, 0, Qt::AlignLeft);
		QFrame* pTableFrame = new QFrame(this);
		QGridLayout* playout = new QGridLayout(pTableFrame);
		playout->setContentsMargins(0, 0, 0, 0);
		playout->setSpacing(0);
        SearchResultTableWdg* pTableWdg = new SearchResultTableWdg(pcInfo.strName, pcInfo.strIp, pcInfo.mapPathChange, this);
		QScrollBar* bar = new QScrollBar(pTableFrame);
		bar->setMaximum(0);
		playout->addWidget(pTableWdg, 0, 0);
		playout->addWidget(bar, 0, 1);
		//添加窗口
		ui.stackedWidget->addWidget(pTableFrame);

		//新建状态显示按钮
		QListWidgetItem *pItem = new QListWidgetItem(pcInfo.strName);
		pItem->setSizeHint(QSize(100, 50));
		ui.pathLstWdg->addItem(pItem);

        sDataUI dataUI;
        dataUI.pcInfo = pcInfo;
        dataUI.pEverythingObj = new EverythingObj(pcInfo.strName, pcInfo.strIp, pcInfo.iPort, this);
        dataUI.pBtn = pBtn;
		dataUI.pItem = pItem;
		dataUI.pTableWdg = pTableWdg;
		dataUI.pBar = bar;
        m_lstDataUI.append(dataUI);

		connect(pBtn, &QPushButton::clicked, this, [&, pBtn, pTableFrame]() {
			ui.stackedWidget->setCurrentWidget(pTableFrame);
			pTableFrame->show();
			for (auto dataUI : m_lstDataUI)
			{
				if (dataUI.pBtn == pBtn)
				{
					dataUI.pBtn->setStyleSheet(
						"QPushButton{border:0px;border-radius:0px;border-top:3px solid rgba(0, 173, 239, 255);background:rgb(250, 250, 250);}"
						"QPushButton:hover{color:rgb(0, 173, 239); }"
						"QPushButton:pressed{color:rgb(0, 173, 239); }"
					);
					int iTotalCount = dataUI.pTableWdg->TotalCount();
					int iAccessCount = dataUI.pTableWdg->AccessCount();
					int iUnAccessCount = dataUI.pTableWdg->UnAccessCount();
					ui.resultLbl->setText(QString::fromLocal8Bit("%1个对象, %2可达, %3不可达.")
						.arg(iTotalCount).arg(iAccessCount).arg(iUnAccessCount));
				}
				else
				{
					dataUI.pBtn->setStyleSheet(
						"QPushButton{border:0px;border-radius:0px;border-top:3px solid rgb(210, 210, 210);background:rgb(250, 250, 250);}"
						"QPushButton:hover{border-top:3px solid rgba(0, 173, 239, 255);color:rgb(0, 173, 239); }"
						"QPushButton:pressed{border-top:3px solid  rgba(0, 173, 239, 50);color:rgb(0, 173, 239); }"
					);
				}
			}
			}, Qt::QueuedConnection);
		
		connect(dataUI.pEverythingObj, &EverythingObj::signalRecvCount, this, [&](int iCount) {
			m_iResultCount += iCount;
		}, Qt::QueuedConnection);
		connect(dataUI.pEverythingObj, &EverythingObj::signalShowRunLog, this,
			[&](QString msg, QColor color) {showRunLog(msg, color); }, Qt::QueuedConnection);
		connect(dataUI.pEverythingObj, &EverythingObj::signalAddResult, this,
			[&](const QStringList lstTargetString, const sSearchResult result, bool isEnd) {
				for (auto &dataUI : m_lstDataUI)
				{
					if (dataUI.pcInfo.strIp == result.strIP)
					{
						int iVisibleRows = dataUI.pTableWdg->visibleRows();
						dataUI.iCount += result.lstFile.count();
						dataUI.pBar->setMaximum(dataUI.iCount - iVisibleRows);
						dataUI.pTableWdg->addResult(lstTargetString, result, isEnd);

						//emit showRunLog(QString::fromLocal8Bit("%1, %2.").arg(result.lstFile.count()).arg(dataUI.iCount));
						if (isEnd)
							dataUI.iCount = 0;
					}
				}
				m_iCurrentCount += result.lstFile.count();
				int rate = m_iCurrentCount * 100 / m_iResultCount;
				ui.progressBar->setValue(rate);
			}, Qt::QueuedConnection);
		connect(dataUI.pBar, &QScrollBar::valueChanged, this, [&](int value) {
			int index = ui.stackedWidget->currentIndex();
			if (m_lstDataUI.count() <= index)
				return;
			m_lstDataUI.at(index).pTableWdg->showRect(value);
			}, Qt::QueuedConnection);
		connect(dataUI.pEverythingObj, &EverythingObj::signalStatusChange,
			this, [&](QString strIp, int iPort, QAbstractSocket::SocketState state) {
				for (auto& dataUI : m_lstDataUI)
				{
					if (dataUI.pcInfo.strIp == strIp &&
						dataUI.pcInfo.iPort == iPort)
					{
						setStatus(dataUI);
					}
				}
			}, Qt::QueuedConnection);
		connect(dataUI.pEverythingObj, &EverythingObj::signalEtpStatusChange,
			this, [&](QString strIp, int iPort, bool bIsEtpConnect) {
				for (auto& dataUI : m_lstDataUI)
				{
					if (dataUI.pcInfo.strIp == strIp &&
						dataUI.pcInfo.iPort == iPort)
					{
						setStatus(dataUI);
					}
				}
			}, Qt::QueuedConnection);
		
		connect(dataUI.pTableWdg, &SearchResultTableWdg::signalShowRunLog, this,
			[&](QString msg, QColor color) {showRunLog(msg, color); }, Qt::QueuedConnection);
		connect(dataUI.pTableWdg, &SearchResultTableWdg::signalPathStatusChange,
			this, [&](QString strIp, QStringList lstAccessPath) {
				for (auto& dataUI : m_lstDataUI)
				{
					if (dataUI.pcInfo.strIp == strIp)
					{
						setStatus(dataUI);
					}
				}
			}, Qt::QueuedConnection);
		connect(dataUI.pTableWdg, &SearchResultTableWdg::signalAddCount, this,
			[&, pcInfo](int iTotalCount, int iAccessCount, int iUnAccessCount) {
				m_iTotalCount += iTotalCount;
				m_iAccessCount += iAccessCount;
				m_iUnAccessCount += iUnAccessCount;
				ui.resultLbl->setText(QString::fromLocal8Bit("%1个对象, %2可达, %3不可达.")
						.arg(m_iTotalCount).arg(m_iAccessCount).arg(m_iUnAccessCount));
		}, Qt::QueuedConnection);

		dataUI.pEverythingObj->open();
    }

	QHBoxLayout* phlayout = qobject_cast<QHBoxLayout*>(ui.listFrame->layout());
	phlayout->addStretch();
	if (!m_lstDataUI.isEmpty())
	{
		for (auto dataUI : m_lstDataUI)
		{
			if (dataUI.pBtn == m_lstDataUI.first().pBtn)
			{
				dataUI.pBtn->setStyleSheet(
					"QPushButton{border:0px;border-radius:0px;border-top:3px solid rgba(0, 173, 239, 255);background:rgb(250, 250, 250);}"
					"QPushButton:hover{color:rgb(0, 173, 239); }"
					"QPushButton:pressed{color:rgb(0, 173, 239); }"
				);
			}
			else
			{
				dataUI.pBtn->setStyleSheet(
					"QPushButton{border:0px;border-radius:0px;border-top:3px solid rgb(210, 210, 210);background:rgb(250, 250, 250);}"
					"QPushButton:hover{border-top:3px solid rgba(0, 173, 239, 255);color:rgb(0, 173, 239); }"
					"QPushButton:pressed{border-top:3px solid  rgba(0, 173, 239, 50);color:rgb(0, 173, 239); }"
				);
			}
		}
		ui.stackedWidget->setCurrentWidget(m_lstDataUI.first().pTableWdg);
		m_lstDataUI.first().pTableWdg->show();
	}
}

void RemoteEverything::saveUiConfig()
{
	QString strPath = QString(QCoreApplication::applicationDirPath()) + "\\Config\\config.ini";
	QSettings setting(strPath, QSettings::IniFormat);
	setting.setIniCodec(QTextCodec::codecForLocale());

	QByteArray state1 = ui.viewSplitter1->saveState();
	QByteArray state2 = ui.viewSplitter2->saveState();
	QByteArray state3 = ui.viewSplitter3->saveState();

	setting.setValue("Ui/viewSplitter1", state1);
	setting.setValue("Ui/viewSplitter2", state2);
	setting.setValue("Ui/viewSplitter3", state3);
}

void RemoteEverything::setStatus(sDataUI& dataUI)
{
#define CONNECTOKCOLOR	QColor(144,238,144)
#define CONNECTINGCOLOR QColor(132,193,251)
	bool isConnected = (dataUI.pEverythingObj->state() == QAbstractSocket::SocketState::ConnectedState);
	bool isEtpConnected = dataUI.pEverythingObj->isEtpConnect();
	QStringList lstAccessPath = dataUI.pTableWdg->AccessPath();
	if (isConnected && isEtpConnected && !lstAccessPath.isEmpty())
		dataUI.pItem->setBackgroundColor(CONNECTOKCOLOR);
	else
		dataUI.pItem->setBackgroundColor(CONNECTINGCOLOR);
	QString strToolTip = QString::fromLocal8Bit("名称: %1 \r\n\
IP: %2 \r\nPort: %3\r\nConnect:%4\r\nETP Connect:%5\r\n路径:\r\n")
.arg(dataUI.pcInfo.strName).arg(dataUI.pcInfo.strIp).arg(dataUI.pcInfo.iPort)
.arg(isConnected).arg(isEtpConnected);
	for (auto strAccessaPath : lstAccessPath)
		strToolTip += "  " + strAccessaPath + "\r\n";
	strToolTip.chop(2);
	dataUI.pItem->setToolTip(strToolTip);
}

void RemoteEverything::showRunLog(QString msg, QColor color /*= Qt::black*/)
{
	ui.runInfoWdg->showRunLog(msg, color);
}
