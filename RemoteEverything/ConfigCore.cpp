#include "ConfigCore.h"
#include <QSettings>
#include <QTextCodec>
#include <QCoreApplication>

CConfigCore* CConfigCore::m_instance = nullptr;

CConfigCore* CConfigCore::GetKernel()
{
	if (m_instance == nullptr)
	{
		m_instance = new CConfigCore;
	}
	return m_instance;
}

void CConfigCore::DestroyKernel()
{
	if (m_instance)
	{
		delete m_instance;
		m_instance = nullptr;
	}
}

sConfig& CConfigCore::GetConfig()
{
	QMutexLocker lock(&m_mutex);
	return m_config;
}

CConfigCore::CConfigCore(QObject* parent /*= nullptr*/):
	QObject(parent)
{
	LoadConfig();
}

CConfigCore::~CConfigCore()
{}

bool CConfigCore::LoadConfig()
{
	QMutexLocker lock(&m_mutex);
	m_config.lstPCInfo.clear();

	QString strPath = QString(QCoreApplication::applicationDirPath()) + "\\Config\\config.ini";
	QSettings setting(strPath, QSettings::IniFormat);
	setting.setIniCodec(QTextCodec::codecForLocale());

	QStringList lstName = setting.value("PC/List").toStringList();
	for (auto strName : lstName)
	{
		sPCInfo pcInfo;
		QStringList lstPathSource, lstPathTarget;
		setting.beginGroup(strName);
		pcInfo.strName = strName;
		pcInfo.strIp = setting.value("IP").toString();
		pcInfo.iPort = setting.value("Port").toInt();
		lstPathSource = setting.value("SourcePath").toStringList();
		lstPathTarget = setting.value("TargetPath").toStringList();
		int iCount = lstPathSource.count();
		for (int i = 0; i < iCount; i++)
		{
			QString strPathTarget;
			if (lstPathTarget.count() > i)
				strPathTarget = lstPathTarget.at(i);
			pcInfo.mapPathChange.insert(lstPathSource.at(i), strPathTarget);
		}
		setting.endGroup();
		m_config.lstPCInfo.append(pcInfo);
	}

	m_config.state1 = setting.value("Ui/viewSplitter1").toByteArray();
	//ui.viewSplitter1->restoreState(state);
	m_config.state2 = setting.value("Ui/viewSplitter2").toByteArray();
	//ui.viewSplitter2->restoreState(state);
	m_config.state3 = setting.value("Ui/viewSplitter3").toByteArray();
	//ui.viewSplitter3->restoreState(state);

	return true;
}

