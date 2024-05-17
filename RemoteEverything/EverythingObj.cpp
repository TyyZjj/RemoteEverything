#include "EverythingObj.h"
#include <QHostAddress>
#include <QtConcurrent>

//EverythingObj::EverythingObj(QObject* parent /*= nullptr*/)
//	: QObject(parent)
//{
//	qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
//	qRegisterMetaType<sFileInfo>("sFileInfo");
//	qRegisterMetaType<sSearchResult>("sSearchResult");
//}

EverythingObj::EverythingObj(QString strName, QString strIp, int iPort, QObject* parent /*= nullptr*/)
	: QThread(parent),
	m_strName(strName),
	m_strIp(strIp),
	m_iPort(iPort),
	m_bIsEtpConnect(false),
	m_pSocket(nullptr),
	m_pTimer(nullptr)
{
	qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
	qRegisterMetaType<sFileInfo>("sFileInfo");
	qRegisterMetaType<sSearchResult>("sSearchResult");

	start();
}

EverythingObj::~EverythingObj()
{
	close();
	terminate();
}

bool EverythingObj::open()
{
	if (m_strIp.isEmpty() || m_iPort <= 0)
	{
		emit signalShowRunLog(QString::fromLocal8Bit("%1 打开失败.").arg(m_strName), Qt::red);
		return false;
	}

	QStringList lst = m_strIp.split(".");
	if (lst.count() != 4)
	{
		emit signalShowRunLog(QString::fromLocal8Bit("%1 打开失败.").arg(m_strName), Qt::red);
		return false;
	}
		
	for (auto &str : lst)
	{
		bool ret(false);
		int seg = str.toInt(&ret);
		if (!ret || seg < 0 || seg > 255)
		{
			emit signalShowRunLog(QString::fromLocal8Bit("%1 打开失败.").arg(m_strName), Qt::red);
			return false;
		}
	}

	if (!m_pSocket)
	{
		m_pSocket = new QTcpSocket;
		connect(m_pSocket, &QTcpSocket::readyRead, m_pSocket, [&]() {
			while (m_pSocket->bytesAvailable())
			{
				QByteArray data = m_pSocket->readAll();
				parse(data);
			}
		}, Qt::QueuedConnection);
		connect(m_pSocket, static_cast<void(QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), 
			m_pSocket, [&](QAbstractSocket::SocketError err) {
				emit signalShowRunLog(QString::fromLocal8Bit("%1 连接报错(%2).")
					.arg(m_strName).arg((int)err), Qt::red);
		});
		//连接状态
		connect(m_pSocket, &QTcpSocket::connected, this, [&]() {
			if (m_pTimer)
			{
				if (m_pTimer->isActive())
					m_pTimer->stop();
				delete m_pTimer;
				m_pTimer = nullptr;
			}
			emit signalShowRunLog(QString::fromLocal8Bit("%1 连接成功.")
				.arg(m_strName));
			emit signalStatusChange(m_strIp, m_iPort, QAbstractSocket::SocketState::ConnectedState);

		},Qt::QueuedConnection);
		connect(m_pSocket, &QTcpSocket::disconnected, this, [&]() {
			m_bIsEtpConnect = false;
			emit signalEtpStatusChange(m_strIp, m_iPort, false);
			if (m_pTimer == nullptr)
			{
				m_pTimer = new QTimer;
				connect(m_pTimer, &QTimer::timeout, this, [&]() {
					open();
				}, Qt::QueuedConnection);
				m_pTimer->start(5000);
				emit signalShowRunLog(QString::fromLocal8Bit("%1 断开连接.")
					.arg(m_strIp), Qt::red);
				emit signalStatusChange(m_strIp, m_iPort, QAbstractSocket::SocketState::UnconnectedState);
			}
		}, Qt::QueuedConnection);
	}
	bool ret = false;
	if (m_pSocket
		&& m_pSocket->state() == QAbstractSocket::UnconnectedState)
	{
		m_pSocket->connectToHost(QHostAddress(m_strIp), m_iPort);
		ret = m_pSocket->waitForConnected();
	}
	return ret;
}

bool EverythingObj::close()
{
	if (m_pTimer)
	{
		if (m_pTimer->isActive())
			m_pTimer->stop();
		delete m_pTimer;
		m_pTimer = nullptr;
	}
	if (m_pSocket)
	{
		m_pSocket->disconnectFromHost();
		m_pSocket->abort();
		delete m_pSocket;
		m_pSocket = nullptr;
	}
	return true;
}

QAbstractSocket::SocketState EverythingObj::state()
{
	if (m_pSocket)
		return m_pSocket->state();
	return QAbstractSocket::SocketState::UnconnectedState;
}

bool EverythingObj::isRecv()
{
	QMutexLocker lock(&m_mutex);
	return m_bIsRecv;
}

bool EverythingObj::search(const QString& target)
{
	if (target.isEmpty())
		return false;
	if (m_pSocket == nullptr
		|| m_pSocket->state() != QAbstractSocket::ConnectedState)
	{
		return false;
	}
	if (m_pSocket->isWritable())
	{
		m_mutex.lock();
		if (target.contains(" "))
			m_lstTargetString = target.split(" ", QString::SplitBehavior::SkipEmptyParts);
		else
			m_lstTargetString = QStringList() << target;

		QByteArray data = "EVERYTHING SEARCH ";
		data += target.toLocal8Bit();
		data += "\r\nEVERYTHING QUERY\r\n";
		m_pSocket->write(data);
		
		m_bIsRecv = false;
		m_mutex.unlock();
		return true;
	}
	return false;
}

void EverythingObj::run()
{
	while (!isInterruptionRequested())
	{
		parsePartResult();
		QThread::msleep(1);
	}
}

bool EverythingObj::send(const QByteArray& data)
{
	if (m_pSocket == nullptr
		|| m_pSocket->state() != QAbstractSocket::ConnectedState)
	{
		return false;
	}
	if (m_pSocket->isWritable())
	{
		m_pSocket->write(data);
		return true;
	}
	return false;
}

void EverythingObj::parse(const QByteArray& data)
{
	if (!m_bIsEtpConnect)
	{
#define everything_protocol0  "220 Welcome to Everything ETP/FTP\r\n"
#define everything_protocol1  "USER anonymous\r\n"
#define everything_protocol2  "230 Logged on.\r\n"
#define everything_protocol3  "OPTS UTF8 ON\r\n"
#define everything_protocol4  "200 UTF8 mode enabled.\r\n"
#define everything_protocol5  "EVERYTHING SIZE_COLUMN 1\r\n"
#define everything_protocol6  "200 Size column set to (1).\r\n"
#define everything_protocol7  "EVERYTHING DATE_MODIFIED_COLUMN 1\r\n"
#define everything_protocol8  "200 Date modified column set to (1).\r\n"
#define everything_protocol9  "EVERYTHING PATH_COLUMN 1\r\n"
#define everything_protocol10 "200 Path column set to (1).\r\n"

		
		m_protocol_data.append(data);
		if (m_protocol_data.contains(everything_protocol0) &&
			m_protocol_data.indexOf(everything_protocol0) == 0)
		{
			m_protocol_data = m_protocol_data.remove(0, QString(everything_protocol0).count());
			send(everything_protocol1);
		}
		else if (m_protocol_data.contains(everything_protocol2) &&
			m_protocol_data.indexOf(everything_protocol2) == 0)
		{
			m_protocol_data = m_protocol_data.remove(0, QString(everything_protocol2).count());
			send(everything_protocol3);
		}
		else if (m_protocol_data.contains(everything_protocol4) &&
			m_protocol_data.indexOf(everything_protocol4) == 0)
		{
			m_protocol_data = m_protocol_data.remove(0, QString(everything_protocol4).count());
			send(everything_protocol5);
		}
		else if (m_protocol_data.contains(everything_protocol6) &&
			m_protocol_data.indexOf(everything_protocol6) == 0)
		{
			m_protocol_data = m_protocol_data.remove(0, QString(everything_protocol6).count());
			send(everything_protocol7);
		}
		else if (m_protocol_data.contains(everything_protocol8) &&
			m_protocol_data.indexOf(everything_protocol8) == 0)
		{
			m_protocol_data = m_protocol_data.remove(0, QString(everything_protocol8).count());
			send(everything_protocol9);
		}
		else if (m_protocol_data.contains(everything_protocol10) &&
			m_protocol_data.indexOf(everything_protocol10) == 0)
		{
			m_protocol_data.clear();
			m_bIsEtpConnect = true;
			emit signalEtpStatusChange(m_strIp, m_iPort, true);
		}
	}
	else
	{
#define everything_protocol11 "200 Search set to "
#define everything_protocol12 "200-Query results\r\n"
#define everything_protocol13 " RESULT_COUNT "
#define everything_protocol14 "200 End.\r\n"
#define everything_protocol15 "200 End."

		m_mutex.lock();
		m_recvdata.append(data);
		m_mutex.unlock();
	}
}

void EverythingObj::parsePartResult()
{
	m_mutex.lock();
	if (!m_recvdata.isEmpty())
	{
		m_procRecvData.append(m_recvdata);
		m_recvdata.clear();
	}
	QStringList lstTargetStrin = m_lstTargetString;
	m_mutex.unlock();

	if (m_procRecvData.isEmpty())
		return;

	if (m_procRecvData.contains(everything_protocol12) &&
		m_procRecvData.contains(everything_protocol13))//开始接收数据
	{
		int iResultCountIndex = m_procRecvData.indexOf(everything_protocol13);
		int iResultCountEndIndex = m_procRecvData.indexOf("\r\n", iResultCountIndex);
		int iProtocol13Count = QString(everything_protocol13).count();
		QString strResultCount = m_procRecvData.mid(iResultCountIndex + iProtocol13Count, iResultCountEndIndex - iResultCountIndex - iProtocol13Count);
		int iResultCount = strResultCount.toInt();
		m_procRecvData = m_procRecvData.remove(0, iResultCountEndIndex + 2);
		m_mutex.lock();
		m_bIsRecv = true;
		m_mutex.unlock();
		emit signalRecvCount(iResultCount);
		emit signalShowRunLog(QString::fromLocal8Bit("%1 收到 %2 条.")
			.arg(m_strName).arg(strResultCount));
	}

	m_mutex.lock();
	bool bIsRecv = m_bIsRecv;
	m_mutex.unlock();
	if (bIsRecv)
	{
		sSearchResult searchResult;
		searchResult.strIP = m_strIp;
		int iLineIndex(0), index(-2), iCount(0), iLastParseSucIndex(0);
		QString str1, str2, str3, str4;
		int iTargetIndex = m_procRecvData.lastIndexOf("\r\n");
		if (iTargetIndex < 0)
			return;

		QByteArray dataTemp = m_procRecvData;
		int indexProtocol14 = dataTemp.indexOf(everything_protocol14);
		if (indexProtocol14 >= 0)
		{
			dataTemp = dataTemp.left(indexProtocol14);
			m_procRecvData = m_procRecvData.remove(0, indexProtocol14 + QString(everything_protocol14).count());
		}

		while (iCount < iTargetIndex)
		{
			int index2 = dataTemp.indexOf("\r\n", index + 2);
			if (index2 < 0)
				break;

			QString str = dataTemp.mid(index + 2, index2 - index - 2);
			switch (iLineIndex)
			{
			case 0:
				str1 = str;
				break;
			case 1:
				str2 = str;
				break;
			case 2:
				str3 = str;
				break;
			case 3:
				str4 = str;
				break;
			default:
				break;
			}

			index = index2;
			iCount = index2 + 2;
			iLineIndex++;
			if ((iLineIndex % 4) == 0)
			{
				iLineIndex = 0;
				iLastParseSucIndex = iCount;

				auto GetSearchResult = [](QString str1, QString str2,
					QString str3, QString str4, sFileInfo& file)->bool {
						auto GetSearchResultPart = [](QString str, sFileInfo& file)->bool {
							if (str.startsWith(" FILE "))
							{
								file.iType = sFileInfo::nFileType::nFile;
								file.strName = str.remove(" FILE ");
								return true;
							}
							else if (str.startsWith(" FOLDER "))
							{
								file.iType = sFileInfo::nFileType::nFolder;
								file.strName = str.remove(" FOLDER ");
								return true;
							}
							else if (str.startsWith(" PATH "))
							{
								file.strPath = str.remove(" PATH ");
								return true;
							}
							else if (str.startsWith(" SIZE "))
							{
								file.iSize = str.remove(" SIZE ").toULongLong();
								return true;
							}
							else if (str.startsWith(" DATE_MODIFIED "))
							{
								long long lastModified = str.remove(" DATE_MODIFIED ").toLongLong();
								lastModified = (lastModified - 116444736000000000ULL) / 10000;
								file.lastModified = QDateTime::fromMSecsSinceEpoch(lastModified);
								//int y = file.lastModified.date().year();
								//int m = file.lastModified.date().month();
								//int d = file.lastModified.date().day();
								//int h = file.lastModified.time().hour();
								return true;
							}
							return false;
						};
						bool ret(true);
						ret &= GetSearchResultPart(str1, file);
						ret &= GetSearchResultPart(str2, file);
						ret &= GetSearchResultPart(str3, file);
						ret &= GetSearchResultPart(str4, file);
						return ret;
				};

				sFileInfo file;
				bool ret = GetSearchResult(str1, str2, str3, str4, file);
				if (!ret)
					continue;
				if (file.strName.isEmpty())
					continue;
				searchResult.lstFile.append(file);
				if (searchResult.lstFile.count() >5000)
				{
					if (iCount < iTargetIndex)
						emit signalAddResult(lstTargetStrin, searchResult);
					else
						emit signalAddResult(lstTargetStrin, searchResult, true);
					searchResult.lstFile.clear();
				}
			}
		}
		if (searchResult.lstFile.count())
		{
			if (indexProtocol14 >= 0)
				emit signalAddResult(lstTargetStrin, searchResult, true);
			else
				emit signalAddResult(lstTargetStrin, searchResult);
		}

		if (indexProtocol14 < 0)
		{
			m_procRecvData = dataTemp.remove(0, iLastParseSucIndex);
		}
		else
		{
			m_mutex.lock();
			m_bIsRecv = false;
			m_mutex.unlock();
		}
	}
}
