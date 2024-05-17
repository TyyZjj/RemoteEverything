#pragma once

#include <QtWidgets/QWidget>
#include <QPushButton>
#include "ui_RemoteEverything.h"
#include "ConfigCore.h"
#include "EverythingObj.h"
#include "SearchResultTableWdg.h"

struct sDataUI
{
    int                     iCount = 0;
    sPCInfo                 pcInfo;
    EverythingObj*          pEverythingObj = nullptr;
    QPushButton*            pBtn = nullptr;
    QListWidgetItem*        pItem = nullptr;
    SearchResultTableWdg*   pTableWdg = nullptr;
    QScrollBar*             pBar = nullptr;
};

//��ҪԶ�̸�Ŀ¼�������Ȩ��
//ʡ����smb��֤, ����Ҫ���ڱ����������벢����Զ���ļ��е��û��������벢��¼
/*20240410
Ŀǰ����һЩ����
1�������뵥���ַ�ʱ, ���ܻ���Ϊ���ںܶ�ƥ���ļ�, �������ݴ���ʱ��ܳ�, ���ݽ���Ҳ�ܺ�ʱ.
  ����Ӧ����Ϊʵʱ����
  ��ʱ������Ŀ���ַ�����Ϊ3���ַ����ϲ��ܿ�ʼ����, ��ֹ����̫�����ݵ��¿���
2����ʱ������Ǻ�ԭ��, �Լ����Ե���̨�豸A-B֮��, A->B����Everything��������, B->A����ʱ, B��������ֱ�Ӳ���������Ҫ�������Բ���.
  ����ԭ����۲�
*/
class RemoteEverything : public QWidget
{
    Q_OBJECT

public:
    RemoteEverything(QWidget *parent = Q_NULLPTR);
    ~RemoteEverything();

public Q_SLOTS:
    void on_searchBtn_clicked();
    void on_copyCurrentBtn_clicked();
    void on_copyBtn_clicked();

Q_SIGNALS:

protected:
    void closeEvent(QCloseEvent* event);

private:
    void initWidget();
    void initDataUI(const sConfig &config);
    void saveUiConfig();

    void setStatus(sDataUI& dataUI);
    void showRunLog(QString msg, QColor color = Qt::black);

private:
    Ui::RemoteEverythingClass ui;

private:
    QList<sDataUI> m_lstDataUI;
    int m_iResultCount; //�ܹ��Ľ����
    int m_iCurrentCount;//��ǰ�ѷ����Ľ����

    int m_iTotalCount; //������յ��Ľ����
    int m_iAccessCount;
    int m_iUnAccessCount;
};
