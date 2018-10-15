 #ifndef MYWIDGET_H
#define MYWIDGET_H

#include <QWidget>
#include <QMultimedia>
#include <QMediaPlayer>
#include <QSlider>
#include <QModelIndex>
class QLabel;
class MyPlaylist;
class MyLrc;

namespace Ui {
class MyWidget;
}

class MyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MyWidget(QWidget *parent = 0);
    ~MyWidget();

private slots:
    void updateTime(qint64 time);
    void setPaused();
    void skipBackward();
    void skipForward();
    void openFile();
    void setPlaylistShown();
    void setLrcShown();
    void sourceChanged(QMediaContent &source);
    void aboutToFinish(QMediaPlayer::MediaStatus state);
    void metaStateChanged(QMediaPlayer::MediaStatus newState);
    void tableClicked(QModelIndex index);
    void clearSources();

private:
    Ui::MyWidget *ui;
    void initPlayer();
    QMediaPlayer *mediaObject;
    QAction *playAction;
    QAction *stopAction;
    QAction *skipBackwardAction;
    QAction *skipForwardAction;
    QLabel *topLabel;
    QLabel *timeLabel;
    MyPlaylist *playlist;
    QMediaPlayer *metaInformationResolver;
    QList<QMediaContent>sources;
    void changeActionState();
    MyLrc *lrc;
    QMap<qint64,QString>lrcMap;
    void resolveLrc(const QString &sourceFileName);
};

#endif // MYWIDGET_H
