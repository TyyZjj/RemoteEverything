#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QMutex>
#include <QTimer>
#include <QDateTime>
#include <QList>
#include <QColor>
#include <QThread>

struct sFileInfo
{
	enum nFileType
	{
		nFile = 0,
		nFolder,
	};
	unsigned short		iType;		//文件类型
	QString				strName;	//名称
	QString				strPath;	//类型
	unsigned long long	iSize;		//大小
	QDateTime			lastModified;	//修改时间
};
inline QDataStream& operator<<(QDataStream& out, const sFileInfo& fileinfo) {
	out << fileinfo.iType << fileinfo.strName 
		<< fileinfo.strPath << fileinfo.iSize << fileinfo.lastModified;
	return out;
}
inline QDataStream& operator>>(QDataStream& in, sFileInfo& fileinfo) {
	in >> fileinfo.iType >> fileinfo.strName
		>> fileinfo.strPath >> fileinfo.iSize >> fileinfo.lastModified;
	return in;
}
Q_DECLARE_METATYPE(sFileInfo)

struct sSearchResult
{
	QString strIP;
	QList<sFileInfo> lstFile;
};
inline QDataStream& operator<<(QDataStream& out, const sSearchResult& result) {
	out << result.strIP << result.lstFile;
	return out;
}
inline QDataStream& operator>>(QDataStream& in, sSearchResult& result) {
	in >> result.strIP >> result.lstFile;
	return in;
}
Q_DECLARE_METATYPE(sSearchResult)


class EverythingObj : public QThread
{
	Q_OBJECT

public:
	//EverythingObj(QObject* parent = nullptr);
	EverythingObj(QString strName, QString strIp, int iPort, QObject *parent = nullptr);
	~EverythingObj();

	bool open();
	bool close();
	
	QAbstractSocket::SocketState state();
	bool isEtpConnect() { return m_bIsEtpConnect; }
	bool isRecv();

	bool search(const QString& target);

Q_SIGNALS:
	void signalRecvCount(int iCount);
	void signalShowRunLog(QString msg, QColor color = Qt::black);
	void signalAddResult(const QStringList lstTargetString, const sSearchResult result, bool isEnd = false);
	void signalStatusChange(QString strIp, int iPort, QAbstractSocket::SocketState state);
	void signalEtpStatusChange(QString strIp, int iPort, bool bIsEtpConnect);

protected:
	void run();

private:
	bool send(const QByteArray& data);
	void parse(const QByteArray& data);
	void parsePartResult();

private:
	QString m_strName;
	QString m_strIp;
	int m_iPort;

	QByteArray		m_protocol_data;
	QByteArray		m_procRecvData;

	bool			m_bIsEtpConnect;			//是否连接了ETP
	QTcpSocket*		m_pSocket;
	QTimer*			m_pTimer;
	bool			m_bIsRecv = false;			//是否正在接收数据

	QMutex			m_mutexNew;
	QByteArray		m_newRecvData;

	QMutex			m_mutex;
	
	QByteArray		m_recvdata;
	
	QStringList		m_lstTargetString;			//目标字符串	
};
