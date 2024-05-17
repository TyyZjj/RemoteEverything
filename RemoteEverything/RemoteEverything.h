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

//需要远程根目录授予访问权限
//省略了smb认证, 主机要先在本主机上输入并保存远端文件夹的用户名、密码并登录
/*20240410
目前存在一些问题
1、当输入单个字符时, 可能会因为存在很多匹配文件, 导致数据传输时间很长, 数据解析也很耗时.
  后续应当改为实时处理
  暂时将搜索目标字符串改为3个字符以上才能开始检索, 防止检索太多数据导致卡顿
2、暂时不清楚是何原因, 自己测试的两台设备A-B之间, A->B传输Everything数据正常, B->A传输时, B的网卡会直接不工作。需要重启电脑才行.
  后续原因待观察
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
    int m_iResultCount; //总共的结果数
    int m_iCurrentCount;//当前已发出的结果数

    int m_iTotalCount; //表格已收到的结果数
    int m_iAccessCount;
    int m_iUnAccessCount;
};
