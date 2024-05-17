#pragma once
#include <QObject>
#include <QMutex>
#include <QMap>

struct sPCInfo
{
	QString strName;
	QString strIp;
	int		iPort;
	QMap<QString, QString> mapPathChange;//QMap<Ô­Â·¾¶, Ìæ»»Â·¾¶>
};
struct sConfig
{
	QList<sPCInfo> lstPCInfo;
	QByteArray state1;
	QByteArray state2;
	QByteArray state3;
};


class CConfigCore : public QObject
{
	Q_OBJECT
public:
	static CConfigCore* GetKernel();
	static void DestroyKernel();

	sConfig& GetConfig();
private:
	CConfigCore(QObject* parent = nullptr);
	~CConfigCore();

	bool LoadConfig();

private:
	static CConfigCore* m_instance;

	QMutex	m_mutex;
	sConfig m_config;
};