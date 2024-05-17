#pragma once
#include <QDir>
#include <QIcon>
#include <QMutex>
#include <QLabel>
#include <QThread>
#include <QPixmap>
#include <QMimeData>
#include <QKeyEvent>
#include <QClipboard>
#include <QHeaderView>
#include <QTableWidget>
#include <QApplication>
#include <QHBoxLayout>
#include <QtWin>
#include <Windows.h>
#include <ShlObj.h>
#include "EverythingObj.h"

#define ROLE_FILE_RESULT	(Qt::UserRole + 101)

struct sTableLine
{
	QFrame*				pNameFrame = nullptr;
	QTableWidgetItem*	pItemPath = nullptr;
	QTableWidgetItem*	pItemSize = nullptr;
	QTableWidgetItem*	pItemTime = nullptr;
	
	sFileInfo fileInfo;

	bool	isPathAccess = false;
	int		iRow = -1;
	QString	strName;
	QString	strPath;
	QString strAccessedPath;
	QString strSize;
	QString strTime;
	QIcon	icon;
};

class SearchResultTableWdg : public QTableWidget
{
	Q_OBJECT

public:
	enum nColIndex
	{
		nFileNameCol = 0,
		nFilePathCol,
		nFileSizeCol,
		nFileTimeCol,
	};

	SearchResultTableWdg(QString strName, QString strIp,
		QMap<QString, QString> mapPathChange = QMap<QString, QString>(),
		QWidget* parent = Q_NULLPTR);
	~SearchResultTableWdg();

	bool isPathExist();
	QStringList AccessPath();
	QList<QUrl> getAccessUrl();
	void showRect(int iStartRow = -1);
	int visibleRows() { return m_iVisibleRows; }

	int TotalCount();
	int AccessCount();
	int UnAccessCount();

	void clear();

Q_SIGNALS:
	void signalAddCount(int iTotalCount, int iAccessCount, int iUnAccessCount);
	void signalShowRunLog(QString msg, QColor color = Qt::black);
	void signalPathStatusChange(QString strIp, QStringList lstAccessPath);

public Q_SLOTS:
	void addResult(const QStringList lstTargetString, const sSearchResult result, bool isEnd= false);

protected:
	void keyPressEvent(QKeyEvent* event);
	void resizeEvent(QResizeEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	void initPathAccessThread();//可达路径初始化
	void initThread();
	void initTimer();
	void initWidget();
	sTableLine createLine(const sFileInfo& fileInfo,
		const QStringList& lstTargetString,
		const QStringList& lstAccessPath);
	void createLineWidget(sTableLine& tableWidget);

	QString GetSizeString(unsigned long long iSize);
	bool isPathAccess(const QString strPath,
		const QStringList lstAccessPath,
		QString& strAccessedPath);
	QString GetShowStr(const QString str, const QStringList lstTargetString);
	QIcon GetFileIcon(const QString& filePath, unsigned short iFileType);
	
private:
	QString			m_strName;
	QString			m_strIp;			//IP
	QMap<QString, QString> m_mapPathChange;

	QMutex			m_mutexPath;
	volatile bool	m_bContinue = true;
	QStringList		m_lstAccessPath;	//可以连接的路径
	std::thread*	m_pPathAccessThread;//判断路径是否存在线程

	QMutex m_mutexCount;
	int m_iTotalCount = 0;
	int m_iAccessCount = 0;
	int m_iUnAccessCount = 0;

	int m_iStartRow;		//当前显示的开始行
	int m_iVisibleRows;		//当前窗口大小能显示的行数
	QTimer* m_pTimer;
	sSearchResult		m_searchResult;	//检索结果
	QList<sTableLine>	m_lstToShowLine;//待显示

	QMutex				m_mutex;
	sSearchResult		m_toProcResult;	//待处理的结果
	QStringList			m_lstTargetString;	//检索的目标字符
	QList<sTableLine>	m_lstLine;		//当前产品所有行的数据
	std::thread*		m_pThread;		//处理待显示结果的线程
};
