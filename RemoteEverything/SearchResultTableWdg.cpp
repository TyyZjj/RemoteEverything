#include "SearchResultTableWdg.h"
#include <QMenu>
#include <QScrollBar>
#include <QProcess>
#include <QDesktopServices>
#include <windows.h>

SearchResultTableWdg::SearchResultTableWdg(QString strName, QString strIp,
	QMap<QString, QString> mapPathChange /*= QMap<QString, QString>()*/,
	QWidget* parent /*= Q_NULLPTR*/)
	: QTableWidget(parent),
	m_strName(strName),
	m_strIp(strIp),
	m_mapPathChange(mapPathChange),
	m_pPathAccessThread(nullptr),
	m_iStartRow(0),
	m_iVisibleRows(0),
	m_pTimer(nullptr)
{
	setStyleSheet(QString::fromUtf8("QLabel{\n"
		"color: rgba(34, 34, 34, 255);\n"
		"font: 18px;\n"
		"font - family: \"Microsoft YaHei\";\n"
		"background:transparent;\n"
		"}\n"
		"QLabel:hover{\n"
		"color:rgb(0, 173, 239);\n"
		"font: 18px;\n"
		"font - family: \"Microsoft YaHei\";\n"
		"background:transparent; }\n"
		"QLabel:pressed{ color:rgb(0, 173, 239);\n"
		"font: 18px;\n"
		"font - family: \"Microsoft YaHei\";\n"
		"background:transparent; }\n"
		"QFrame:hover{\n"
		"background - color: rgba(0, 173, 239);\n"
		"}\n"
		"QFrame:pressed{\n"
		"background - color: rgba(0, 173, 239);\n"
		"}"));

	initPathAccessThread();
	initThread();
	initTimer();
	initWidget();

	setSelectionBehavior(QAbstractItemView::SelectRows);
	verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	//verticalHeader()->setVisible(false); //设置垂直头不可见
	horizontalHeader()->setSectionsClickable(false);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	QStringList header = QStringList() << QString::fromLocal8Bit("名称")
		<< QString::fromLocal8Bit("路径")
		<< QString::fromLocal8Bit("大小")
		<< QString::fromLocal8Bit("修改时间");
	setColumnCount(header.count());
	verticalScrollBar()->setVisible(false);
	setHorizontalHeaderLabels(header);
	setWordWrap(false);
	setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

	setColumnWidth(nFileNameCol, 200);
	setColumnWidth(nFilePathCol, 600);
	setColumnWidth(nFileSizeCol, 100);
	setColumnWidth(nFileTimeCol, 150);
}


SearchResultTableWdg::~SearchResultTableWdg()
{
	if (m_pTimer)
	{
		if (m_pTimer->isActive())
			m_pTimer->stop();
		delete m_pTimer;
		m_pTimer = nullptr;
	}

	if (m_pPathAccessThread)
	{
		m_bContinue = false;
		m_pPathAccessThread->detach();
		delete m_pPathAccessThread;
		m_pPathAccessThread = nullptr;
	}

	if (m_pThread)
	{
		m_bContinue = false;
		m_pThread->detach();
		delete m_pThread;
		m_pThread = nullptr;
	}
}

bool SearchResultTableWdg::isPathExist() {
	QMutexLocker lock(&m_mutexPath);
	return m_lstAccessPath.isEmpty();
}

QStringList SearchResultTableWdg::AccessPath()
{
	QMutexLocker lock(&m_mutexPath);
	return m_lstAccessPath;
}

QList<QUrl> SearchResultTableWdg::getAccessUrl()
{
	QList<QUrl> lstUrl;
	for (auto fileInfo : m_searchResult.lstFile)
	{
		QString strAccessedPath;
		if (isPathAccess(fileInfo.strPath + "\\" + fileInfo.strName, m_lstAccessPath, strAccessedPath))
			lstUrl.append(QUrl::fromLocalFile(strAccessedPath));
	}
	return lstUrl;
	//QMimeData* mimeData = new QMimeData;
	//mimeData->setUrls(lstUrl);
	//QApplication::clipboard()->setMimeData(mimeData);
}

void SearchResultTableWdg::showRect(int iStartRow)
{
	if (iStartRow < 0)
		iStartRow = m_iStartRow;
	else
		m_iStartRow = iStartRow;
	int iRowCount = rowCount();
	if (iRowCount && iStartRow < iRowCount)
	{
		int iRowHeight = rowHeight(0);
		QRect visibleRect = viewport()->rect();
		m_iVisibleRows = visibleRect.height() / iRowHeight; //计算可见的行数
		int iEndRow = iStartRow + m_iVisibleRows + 1;

		m_mutexPath.lock();
		QStringList lstAccessPath = m_lstAccessPath;
		m_mutexPath.unlock();

		m_mutex.lock();
		QStringList lstTargetString = m_lstTargetString;
		QList<sTableLine> lstLine = m_lstLine;
		m_mutex.unlock();

		for (int iRow = 1/*第0行要作为标志行*/; iRow < iStartRow; iRow++)
		{
			setRowHidden(iRow, true);
		}
		for (int iRow = iEndRow; iRow < iRowCount; iRow++)
		{
			setRowHidden(iRow, true);
		}

		QList<sTableLine> lstToShowLine;
		for (int iRow = iStartRow; iRow < iEndRow; iRow++)
		{
			if (iRow < 0 || iRow >= iRowCount)
				continue;

			setRowHidden(iRow, false);
			QTableWidgetItem* itemSize = item(iRow, nFileSizeCol);
			if (itemSize == nullptr)
			{
				sTableLine tableLine;
				if (lstLine.count() > iRow)
				{
					tableLine = lstLine.at(iRow);
					createLineWidget(tableLine);
				}
				else
				{
					const sFileInfo& fileInfo = m_searchResult.lstFile.at(iRow);
					tableLine = createLine(fileInfo, lstTargetString, lstAccessPath);
					createLineWidget(tableLine);
				}
				tableLine.iRow = iRow;
				lstToShowLine.append(tableLine);
			}
		}
		m_lstToShowLine = lstToShowLine;
	}
}

int SearchResultTableWdg::TotalCount() {
	QMutexLocker lock(&m_mutexCount);
	return m_iTotalCount;
}
int SearchResultTableWdg::AccessCount() {
	QMutexLocker lock(&m_mutexCount);
	return m_iAccessCount;
}
int SearchResultTableWdg::UnAccessCount() {
	QMutexLocker lock(&m_mutexCount);
	return m_iUnAccessCount;
}

void SearchResultTableWdg::clear()
{
	//if (m_pTimer)
	//{
	//	if (m_pTimer->isActive())
	//		m_pTimer->stop();
	//	delete m_pTimer;
	//	m_pTimer = nullptr;
	//}

	clearContents();
	setRowCount(0);
	
	m_searchResult.lstFile.clear();
	m_mutex.lock();
	m_toProcResult.lstFile.clear();
	m_lstTargetString.clear();
	m_lstLine.clear();
	m_mutex.unlock();

	m_mutexCount.lock();
	m_iTotalCount = 0;
	m_iAccessCount = 0;
	m_iUnAccessCount = 0;
	m_mutexCount.unlock();

	m_iStartRow = 0;
	m_iVisibleRows = 0;
}

void SearchResultTableWdg::addResult(const QStringList lstTargetString, 
	const sSearchResult result, 
	bool isEnd /*= false*/)
{
	m_mutexPath.lock();
	QStringList lstAccessPath = m_lstAccessPath;
	m_mutexPath.unlock();

	int iRowCount = rowCount();
	m_mutex.lock();
	m_toProcResult.lstFile.append(result.lstFile);
	m_lstTargetString = lstTargetString;
	//std::fill_n(std::back_inserter(m_lstLine), result.lstFile.count(), sTableLine());
	m_mutex.unlock();
	m_searchResult.lstFile.append(result.lstFile);

	m_mutexCount.lock();
	m_iTotalCount += result.lstFile.count();
	m_mutexCount.unlock();
	
	setRowCount(iRowCount + result.lstFile.count());
	if (iRowCount == 0)
		showRect(0);
}

void SearchResultTableWdg::keyPressEvent(QKeyEvent* event)
{
	if (event->modifiers() == Qt::KeyboardModifier::ControlModifier &&
		event->key() == Qt::Key::Key_C)//ctrl+c
	{
		m_mutexPath.lock();
		QStringList lstAccessPath = m_lstAccessPath;
		m_mutexPath.unlock();

		QList<QTableWidgetSelectionRange> lstRange = this->selectedRanges();
		QList<QUrl> lstUrl;
		for (QTableWidgetSelectionRange& range : lstRange)
		{
			int topRow = range.topRow();
			int bottomRow = range.bottomRow();
			int leftColumn = range.leftColumn();
			int rightColumn = range.rightColumn();
			for (int row = topRow; row <= bottomRow; row++)
			{
				sFileInfo fileInfo = item(row, nFileSizeCol)->data(ROLE_FILE_RESULT).value<sFileInfo>();
				QString strAccessedPath;
				if (isPathAccess(fileInfo.strPath + "\\" + fileInfo.strName, lstAccessPath, strAccessedPath))
					lstUrl.append(QUrl::fromLocalFile(strAccessedPath));
			}
		}
		QMimeData* mimeData = new QMimeData;
		mimeData->setUrls(lstUrl);
		QApplication::clipboard()->setMimeData(mimeData);
	}
	else if (event->modifiers() == Qt::KeyboardModifier::ControlModifier &&
		event->key() == Qt::Key::Key_A)
	{
		this->selectAll();
	}
	else if (event->key() == Qt::Key::Key_Delete)
	{
		QList<QTableWidgetSelectionRange> lstRange = this->selectedRanges();
		for (QTableWidgetSelectionRange& range : lstRange)
		{
			int topRow = range.topRow();
			int bottomRow = range.bottomRow();
			int leftColumn = range.leftColumn();
			int rightColumn = range.rightColumn();
			for (int row = topRow; row <= bottomRow; row++)
			{
				for (int column = leftColumn; column <= rightColumn; column++)
				{
					QTableWidgetItem* item = this->item(row, column);
					item->setText("");
				}
			}
		}
	}
}

void SearchResultTableWdg::resizeEvent(QResizeEvent* event)
{
	int iRowCount = rowCount();
	if (iRowCount)
	{
		int iRowHeight = rowHeight(0);
		QRect visibleRect = viewport()->rect();
		int iVisibleRows = visibleRect.height() / iRowHeight;
		if (iVisibleRows != m_iVisibleRows)
			showRect(m_iStartRow);
	}
	
	return QTableWidget::resizeEvent(event);
}

void SearchResultTableWdg::wheelEvent(QWheelEvent* event)
{
	event->ignore();
}

//void SearchResultTableWdg::wheelEvent(QWheelEvent* event)
//{
//	createCurrentRect();
//	return QTableWidget::wheelEvent(event);
//}

void SearchResultTableWdg::initPathAccessThread()
{
	m_pPathAccessThread = new std::thread([=]() {
		int iCount(0);
		while (m_bContinue)
		{
			if (iCount % 200 == 0)
			{
				QDir dir;
				dir.setPath(QString("\\\\") + m_strIp + "\\");
				if (dir.exists())
				{
					QStringList lst = dir.entryList();
					m_mutexPath.lock();
					int iCount = m_lstAccessPath.count();
					m_lstAccessPath.clear();
					for (auto str : lst)
					{
						str = QString("\\\\") + m_strIp + "\\" + str;
						m_lstAccessPath.append(str);
					}
					if (iCount != m_lstAccessPath.count())
						emit signalPathStatusChange(m_strIp, m_lstAccessPath);
					m_mutexPath.unlock();
				}
				else
				{
					m_mutexPath.lock();
					if (!m_lstAccessPath.isEmpty())
						emit signalPathStatusChange(m_strIp, QStringList());
					m_lstAccessPath.clear();
					m_mutexPath.unlock();
				}
				iCount = 0;
			}
			iCount++;
			QThread::msleep(100);
		}
		});
}

void SearchResultTableWdg::initThread()
{
	m_pThread = new std::thread([=]() {
		while (m_bContinue)
		{
			m_mutex.lock();
			QList<sFileInfo> lstFile = m_toProcResult.lstFile;
			QStringList lstTargetString = m_lstTargetString;
			m_mutex.unlock();
			if (lstFile.isEmpty())
			{
				QThread::msleep(10);
				continue;
			}

			m_mutexPath.lock();
			QStringList lstAccessPath = m_lstAccessPath;
			m_mutexPath.unlock();

			int iAccessCount(0), iUnAccessCount(0);
			int iCount = lstFile.count();
			for (int index = 0; index < iCount; index++)
			{
				const sFileInfo fileInfo = lstFile.at(index);
				sTableLine tableLine = createLine(fileInfo, lstTargetString, lstAccessPath);
				if (tableLine.isPathAccess)
					iAccessCount++;
				else
					iUnAccessCount++;

				m_mutex.lock();
				if (m_toProcResult.lstFile.count()&&
					m_lstTargetString == lstTargetString)
				{
					m_toProcResult.lstFile.erase(m_toProcResult.lstFile.begin());
					m_lstLine.append(tableLine);
				}
				m_mutex.unlock();
			}

			m_mutexCount.lock();
			m_iAccessCount += iAccessCount;
			m_iUnAccessCount += iUnAccessCount;
			emit signalAddCount(iAccessCount + iUnAccessCount, iAccessCount, iUnAccessCount);
			m_mutexCount.unlock();
		}
	});
}

void SearchResultTableWdg::initTimer()
{
	m_pTimer = new QTimer(this);
	m_pTimer->setInterval(10);
	connect(m_pTimer, &QTimer::timeout, this, [&]() {
		if (!m_lstToShowLine.isEmpty())
		{
			int index = m_lstToShowLine.count() - 1;
			int iRowCount = rowCount();
			//int iCount(0);
			for (; index >= 0; index--)
			{
				//if (iCount < 10)
				//	iCount++;
				//else
				//	break;
			
				const sTableLine tableLine = m_lstToShowLine.at(index);
				if (tableLine.iRow >=0 && tableLine.iRow < iRowCount)
				{
					setCellWidget(tableLine.iRow, nFileNameCol, tableLine.pNameFrame);
					setItem(tableLine.iRow, nFilePathCol, tableLine.pItemPath);
					setItem(tableLine.iRow, nFileSizeCol, tableLine.pItemSize);
					setItem(tableLine.iRow, nFileTimeCol, tableLine.pItemTime);
				}
				m_lstToShowLine.removeLast();
			}
		}
		}, Qt::QueuedConnection);
	m_pTimer->start();
}

void SearchResultTableWdg::initWidget()
{
	connect(this, &SearchResultTableWdg::customContextMenuRequested, this, [&](const QPoint& pos) {
		m_mutexPath.lock();
		QStringList lstAccessPath = m_lstAccessPath;
		m_mutexPath.unlock();
		
		QMenu* popMenu = new QMenu(this);
		QAction* copyNameAction = new QAction(QString::fromLocal8Bit("复制文件名"), popMenu);
		connect(copyNameAction, &QAction::triggered, [&]() {
			QList<QTableWidgetSelectionRange> lstRange = this->selectedRanges();
			QString strCopyData;
			for (QTableWidgetSelectionRange& range : lstRange)
			{
				int topRow = range.topRow();
				int bottomRow = range.bottomRow();
				int leftColumn = range.leftColumn();
				int rightColumn = range.rightColumn();
				for (int row = topRow; row <= bottomRow; row++)
				{
					QTableWidgetItem* item = this->item(row, nFileSizeCol);
					if (!item)
						continue;
					const sFileInfo& fileInfo = item->data(ROLE_FILE_RESULT).value<sFileInfo>();
					strCopyData.append(fileInfo.strName);
					strCopyData.append("\r\n");
				}
			}
			if (strCopyData.count())
				strCopyData.chop(2);
			QClipboard* clipboard = QApplication::clipboard();
			clipboard->setText(strCopyData);
		});

		QAction* copyPathAction = new QAction(QString::fromLocal8Bit("复制路径"), popMenu);
		connect(copyPathAction, &QAction::triggered, [&]() {
			QList<QTableWidgetSelectionRange> lstRange = this->selectedRanges();
			QString strCopyData;
			for (QTableWidgetSelectionRange& range : lstRange)
			{
				int topRow = range.topRow();
				int bottomRow = range.bottomRow();
				int leftColumn = range.leftColumn();
				int rightColumn = range.rightColumn();
				for (int row = topRow; row <= bottomRow; row++)
				{
					QTableWidgetItem* item = this->item(row, nFileSizeCol);
					if (!item)
						continue;
					const sFileInfo& fileInfo = item->data(ROLE_FILE_RESULT).value<sFileInfo>();
					strCopyData.append(fileInfo.strPath);
					strCopyData.append("\r\n");
				}
			}
			if (strCopyData.count())
				strCopyData.chop(2);
			QClipboard* clipboard = QApplication::clipboard();
			clipboard->setText(strCopyData);
		});

		QAction* copyAccessPathAction = new QAction(QString::fromLocal8Bit("复制可达路径"), popMenu);
		connect(copyAccessPathAction, &QAction::triggered, [&, lstAccessPath]() {
			QList<QTableWidgetSelectionRange> lstRange = this->selectedRanges();
			QString strCopyData;
			for (QTableWidgetSelectionRange& range : lstRange)
			{
				int topRow = range.topRow();
				int bottomRow = range.bottomRow();
				int leftColumn = range.leftColumn();
				int rightColumn = range.rightColumn();
				for (int row = topRow; row <= bottomRow; row++)
				{
					QTableWidgetItem* item = this->item(row, nFileSizeCol);
					if (!item)
						continue;
					const sFileInfo &fileInfo = item->data(ROLE_FILE_RESULT).value<sFileInfo>();
					QString strAccessPath;
					if (isPathAccess(fileInfo.strPath, lstAccessPath, strAccessPath))//还是需要实时刷新路径的
					{
						strCopyData.append(strAccessPath);
						strCopyData.append("\r\n");
					}
				}
			}
			if (strCopyData.count())
				strCopyData.chop(2);
			QClipboard* clipboard = QApplication::clipboard();
			clipboard->setText(strCopyData);
		});

		popMenu->addAction(copyNameAction);
		popMenu->addAction(copyPathAction);
		popMenu->addAction(copyAccessPathAction);

		QTableWidgetItem* pItem = itemAt(pos);
		if (!pItem)
			return;
		int iRow = row(pItem);
		pItem = item(iRow, nFileSizeCol);
		if (!pItem)
			return;
		const sFileInfo& fileInfo = pItem->data(ROLE_FILE_RESULT).value<sFileInfo>();
		QString strAccessPath;
		if (isPathAccess(fileInfo.strPath, lstAccessPath, strAccessPath))
		{
			QAction* openPathAction = new QAction(QString::fromLocal8Bit("打开文件路径"), popMenu);
			connect(openPathAction, &QAction::triggered, [&,strAccessPath]() {
				//QFileInfo file(strAccessPath);
				//QString strPath = file.absolutePath();
				//while (strPath.contains("/"))
				//	strPath = strPath.replace("/", "\\");
				ShellExecuteA(nullptr, "open", strAccessPath.toStdString().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
				//QDesktopServices::openUrl(QUrl(QString("file:///%1").arg(strAccessPath)));
			});
			popMenu->addAction(openPathAction);
		}

		if (popMenu)
		{
			//菜单出现位置
			popMenu->exec(QCursor::pos());
			delete popMenu;
			popMenu = nullptr;
		}
	}, Qt::QueuedConnection);
}

sTableLine SearchResultTableWdg::createLine(const sFileInfo& fileInfo, const QStringList& lstTargetString, const QStringList& lstAccessPath)
{
	sTableLine tableLine;
	tableLine.fileInfo = fileInfo;
	unsigned long long	iSize = fileInfo.iSize;
	tableLine.strSize = GetSizeString(iSize);
	tableLine.strName = GetShowStr(fileInfo.strName, lstTargetString);
	QString strAccessedPath;
	tableLine.isPathAccess = isPathAccess(fileInfo.strPath + "\\" + fileInfo.strName, lstAccessPath, strAccessedPath);
	if (tableLine.isPathAccess)
		tableLine.icon = GetFileIcon(strAccessedPath, fileInfo.iType);
	tableLine.strPath = fileInfo.strPath;
	tableLine.strAccessedPath = strAccessedPath;
	tableLine.strTime = fileInfo.lastModified.toString("yyyy/MM/dd hh:mm");

	return tableLine;
}

void SearchResultTableWdg::createLineWidget(sTableLine& tableWidget)
{
	tableWidget.pNameFrame = new QFrame(this);
	QHBoxLayout* hlayout = new QHBoxLayout(tableWidget.pNameFrame);
	hlayout->setContentsMargins(0, 0, 0, 0);
	hlayout->setSpacing(5);
	QLabel* pIconLbl = new QLabel(tableWidget.pNameFrame);
	pIconLbl->setFixedSize(25, 25);
	QLabel* pNameLbl = new QLabel(tableWidget.strName, tableWidget.pNameFrame);
	hlayout->addWidget(pIconLbl);
	hlayout->addWidget(pNameLbl);

	tableWidget.pItemPath = new QTableWidgetItem(tableWidget.strPath);
	QString strAccessedPath;
	if (tableWidget.isPathAccess)
	{
		pIconLbl->setPixmap(tableWidget.icon.pixmap(25, 25));
	}
	else
	{
		tableWidget.pItemPath->setBackgroundColor(QColor(255, 153, 153));
	}
	tableWidget.pItemSize = new QTableWidgetItem(tableWidget.strSize);
	tableWidget.pItemTime = new QTableWidgetItem(tableWidget.strTime);
	tableWidget.pItemSize->setData(ROLE_FILE_RESULT, QVariant::fromValue(tableWidget.fileInfo));
}

QString SearchResultTableWdg::GetSizeString(unsigned long long iSize)
{
	if (iSize >= std::numeric_limits<long long>::max())
		return "";

	int iCount(0);
	do
	{
		if (iSize >= 1024)
		{
			iSize /= 1024;
			iCount++;
		}
		else
			break;
	} while (/*B,K,M,T,P,E,Z,Y*/iCount < 7);

	QString strSize;
	switch (iCount)
	{
	case 0:
		strSize = QString::number(iSize) + "B";
		break;
	case 1:
		strSize = QString::number(iSize) + "K";
		break;
	case 2:
		strSize = QString::number(iSize) + "M";
		break;
	case 3:
		strSize = QString::number(iSize) + "T";
		break;
	case 4:
		strSize = QString::number(iSize) + "P";
		break;
	case 5:
		strSize = QString::number(iSize) + "E";
		break;
	case 6:
		strSize = QString::number(iSize) + "Z";
		break;
	case 7:
		strSize = QString::number(iSize) + "Y";
		break;
	default:
		strSize = QString::number(iSize) + "Y";
		break;
	}
	return strSize;
}

bool SearchResultTableWdg::isPathAccess(const QString strPath,
	const QStringList lstAccessPath,
	QString& strAccessedPath)
{
	QString strPathTemp = QString("\\\\") + m_strIp + "\\" + strPath;
	auto isPathAccessImp = [&]() ->bool {
		for (auto strAccessPath : lstAccessPath)
		{
			if (strPathTemp.contains(strAccessPath))
			{
				strAccessedPath = strPathTemp;
				if (strAccessedPath.contains(":") &&
					strAccessedPath.contains("\\\\"))
					strAccessedPath.remove(":");
				return true;
			}
		}
		return false;
	};
	if (isPathAccessImp())
		return true;

	for (auto it = m_mapPathChange.cbegin(); it != m_mapPathChange.cend(); ++it)
	{
		const QString& strSourcePath = it.key();
		const QString& strTargetPath = it.value();
		if (strPathTemp.contains(strSourcePath))
		{
			strPathTemp = strPathTemp.replace(strSourcePath, strTargetPath);
			if (!strPathTemp.contains("\\\\"))//如果就是当前ip
			{
				if (QFile::exists(strPathTemp))
				{
					strAccessedPath = strPathTemp;
					return true;
				}
				else
					return false;
			}
			//重复之前的操作
			if (isPathAccessImp())
				return true;
		}
	}
	return false;
}

QString SearchResultTableWdg::GetShowStr(const QString str, const QStringList lstTargetString)
{
	QString strPathTemp = str;
	for (auto strTarget : lstTargetString)
	{
		int index(0);
		//do 
		//{
		index = strPathTemp.lastIndexOf(strTarget, -1, Qt::CaseSensitivity::CaseInsensitive);
		if (index != -1)
		{
			QString strPart = strPathTemp.mid(index, strTarget.count());
			QString strPart2 = QString("<b>") + strPart + "</b>";
			strPathTemp = strPathTemp.replace(index, strTarget.count(), strPart2);
		}
		//} while (index != -1);
	}
	return strPathTemp;
}

QIcon SearchResultTableWdg::GetFileIcon(const QString& filePath, unsigned short iFileType) {
	static QMap<QString, QIcon> mapFileIcon;
	static QIcon folderIcon;
	QString strSuffix;
	if (iFileType == sFileInfo::nFile)
	{
		QFileInfo fileInfo(filePath);
		strSuffix = fileInfo.suffix();
		if (!strSuffix.endsWith(".exe") &&
			!strSuffix.endsWith(".lnk") &&
			mapFileIcon.contains(strSuffix))
			return mapFileIcon.value(strSuffix);
	}
	else if (!folderIcon.isNull())
		return folderIcon;

	SHFILEINFO shFileInfo;
	memset(&shFileInfo, 0, sizeof(shFileInfo));

	// 使用SHGetFileInfo获取文件信息，包括图标  
	if (SHGetFileInfo(filePath.toStdWString().c_str(),
		0,
		&shFileInfo,
		sizeof(shFileInfo),
		SHGFI_ICON | SHGFI_LARGEICON)) {
		// 将HICON转换为QIcon  
		QIcon icon;
		icon.addPixmap(QtWin::fromHICON(shFileInfo.hIcon));
		DestroyIcon(shFileInfo.hIcon); // 不要忘记释放图标句柄  

		if (iFileType == sFileInfo::nFile)
			mapFileIcon.insert(strSuffix, icon);
		else
			folderIcon = icon;
		return icon;
	}

	// 如果获取失败，返回一个默认图标  
	return QIcon();
}