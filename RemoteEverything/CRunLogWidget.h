#pragma once
#include <QMutex>
#include <QTime>
#include <QListWidget>
#include <QKeyEvent>
#include <QClipboard>
#include <QApplication>

class CRunLogWidget : public QListWidget
{
public:
	CRunLogWidget(QWidget *parent = nullptr) : QListWidget(parent) {
		setSelectionBehavior(QAbstractItemView::SelectItems);
		setSelectionMode(QAbstractItemView::ExtendedSelection);
	}

public Q_SLOTS:
	void showRunLog(QString msg, QColor color = Qt::black)
	{
		QMutexLocker lock(&m_mutexMsgLst);
		if (color.isValid())
		{
			msg = "[" + QTime::currentTime().toString() + "] " + msg;
			QListWidgetItem* item = new QListWidgetItem(msg);
			item->setTextColor(color);
			insertItem(0, item);
			//addItem(item);
		}

		if (count() > 1000)
		{
			int iCount = count();
			QListWidgetItem* item = this->item(iCount - 1);
			removeItemWidget(item);
			delete item;
		}
		//this->scrollToBottom();
	}

Q_SIGNALS:
protected:
	void keyPressEvent(QKeyEvent *event)
	{
		if (event->modifiers() == Qt::KeyboardModifier::ControlModifier &&
			event->key() == Qt::Key::Key_C)//ctrl+c
		{
			QList<QListWidgetItem*> lstItem = this->selectedItems();
			QString strCopyData;
			for (auto &item : lstItem)
			{
				if (item != nullptr)
				{
					strCopyData.append(item->text());
					strCopyData.append("\r\n");
				}
			}
			strCopyData.chop(1);
			QClipboard *clipboard = QApplication::clipboard();
			clipboard->setText(strCopyData);
		}
		else if (event->modifiers() == Qt::KeyboardModifier::ControlModifier &&
			event->key() == Qt::Key::Key_A)
		{
			this->selectAll();
		}
	}

private:
	QMutex m_mutexMsgLst;
};