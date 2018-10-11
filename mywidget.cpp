#include "mywidget.h"
#include "ui_mywidget.h"
#include <QLabel>
#include <QToolBar>
#include <QVBoxLayout>
#include <QTime>
#include <QSlider>
#include <QAbstractSlider>
#include <myplaylist.h>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QMediaPlaylist>

MyWidget::MyWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MyWidget)
{
    ui->setupUi(this);
    initPlayer();
}

MyWidget::~MyWidget()
{
    delete ui;
}

void MyWidget::updateTime(qint64 time)
{
    qint64 totalTimeValue = mediaObject->duration();//获取媒体总时长，单位为毫秒
    QTime totalTime(0,(totalTimeValue/60000) % 60,(totalTimeValue/1000)%60);
    QTime currentTime(0,(time/60000)%60,(time/1000)%60);
    QString str = currentTime.toString("mm:ss")+"/"+totalTime.toString("mm:ss");
    timeLabel->setText(str);
}
//播放或暂停
void MyWidget::setPaused()
{
    if(mediaObject->state() == QMediaPlayer::PlayingState)
        mediaObject->pause();
    else
        mediaObject->play();
}
//播放上一首
void MyWidget::skipBackward()
{
    int index = sources.indexOf(mediaObject->currentMedia());
    mediaObject->setMedia(sources.at(index-1));
    mediaObject->play();
}
//播放下一首
void MyWidget::skipForward()
{
    int index = sources.indexOf(mediaObject->currentMedia());
    mediaObject->setMedia(sources.at(index+1));
    mediaObject->play();
}
//打开文件
void MyWidget::openFile()
{
    //从系统音乐目录打开多个音乐文件
    QStringList list = QFileDialog::getOpenFileNames(this,tr("打开音乐文件"),
                                                    "/home");
    if(list.isEmpty())
        return;

    //获取当前媒体源列表的大小
    int index = sources.size();

    //将打开的音乐文件添加到媒体源列表后
    foreach(QString string,list){
        QMediaContent source(QUrl::fromLocalFile(string));
        sources.append(source);
    }
    //如果媒体源列表不为空，则将新加入的第一个媒体源作为当前媒体源
    //这是会发射stateChanged()信号，从而调用metaStateChanged()函数进行媒体源的解析
    if(!sources.isEmpty())
        metaInformationResolver->setMedia(sources.at(index));
}
//显示或隐藏播放列表
void MyWidget::setPlaylistShown()
{
    if(playlist->isHidden()){
        playlist->move(frameGeometry().bottomLeft());
        playlist->show();
    }else{
        playlist->hide();
    }
}
//显示或隐藏桌面歌词
void MyWidget::setLrcShown()
{

}
//当媒体源改变时，在播放列表中选中相应的行并更新图标的状态
void MyWidget::sourceChanged(QMediaContent &source)
{
    int index = sources.indexOf(source);
    playlist->selectRow(index);
    changeActionState();
}
//当前媒体源播放将要结束时，如果在列表中当前媒体源的后面还有媒体源
//那么将它添加到播放队列中，否则停止播放
void MyWidget::aboutToFinish()
{
    int index = sources.indexOf(mediaObject->currentMedia())+1;
    if(sources.size()>index){
        mediaObject->playlist()->addMedia(sources.at(index));
    }else{
        mediaObject->stop();
    }
}
//解析媒体文件的元信息
void MyWidget::metaStateChanged(QMediaPlayer::MediaStatus newState)
{
    //错误状态，则从媒体源列表中除去新添加的媒体源
    if(newState == QMediaPlayer::NoMedia){
        QMessageBox::warning(this,tr("打开文件时出错"),metaInformationResolver->errorString());
        while(!sources.isEmpty() && !(sources.takeLast()==
                                      metaInformationResolver->currentMedia()))
        {};
        return;
    }
    //如果既不处于停止状态也不处于暂停状态，则直接返回
    if(newState == QMediaPlayer::LoadingMedia)
        return;
    //如果媒体源类型错误，则直接返回
    if(metaInformationResolver->error() == QMediaPlayer::ResourceError)
        return;
    
    //获取媒体信息
    //QMap<QString,QVariant>metaData = metaInformationResolver->metaData("").toMap();
    
    //获取标题，如果为空，则使用文件名
    QString title = metaInformationResolver->metaData("TITLE").toString();
    if(title == ""){
        QString str = metaInformationResolver->currentMedia().canonicalResource().url().toLocalFile();
        title = QFileInfo(str).baseName();
    }
    QTableWidgetItem *titleItem = new QTableWidgetItem(title);
    
    //设置数据项不可编辑
    titleItem->setFlags(titleItem->flags()^Qt::ItemIsEditable);
    
    //获取艺术家信息
    QTableWidgetItem *artistItem = 
            new QTableWidgetItem(metaInformationResolver->metaData("ARTIST").toString());
    artistItem->setFlags(artistItem->flags()^Qt::ItemIsEditable);
    
    //获取总时间信息
    qint64 totalTime = metaInformationResolver->duration();
    QTime time(0,(totalTime/60000)%60,(totalTime/1000)%60);
    QTableWidgetItem *timeItem = new QTableWidgetItem(time.toString("mm:ss"));
    
    //插入到播放列表
    int currentRow = playlist->rowCount();
    playlist->insertRow(currentRow);
    playlist->setItem(currentRow,0,titleItem);
    playlist->setItem(currentRow,1,artistItem);
    playlist->setItem(currentRow,2,timeItem);
    
    //如果添加的媒体源还没有解析完，那么继续解析下一个媒体源
    int index = sources.indexOf(metaInformationResolver->currentMedia())+1;
    if(sources.size()>index){
        metaInformationResolver->setMedia(sources.at(index));
    }else{//如果所有媒体源都已经解析完成
        //如果播放列表中没有选中的行
        if(playlist->selectedItems().isEmpty()){
            //如果现在没有播放歌曲则设置第一个媒体源为媒体对象的当前媒体源
            //（因为可能正在播放歌曲时清空了播放列表，然后又添加了新的列表）
            if(mediaObject->state() != QMediaPlayer::PlayingState &&
                    mediaObject->state() != QMediaPlayer::PausedState){
                mediaObject->setMedia(sources.at(0));
            }else{
                //如果正在播放歌曲，则选中播放列表的第一个曲目，并更改图标状态
                playlist->selectRow(0);
                changeActionState();
            }
        }
        else{
            //如果播放列表中有选中的行，那么直接更新图标状态
            changeActionState();
        }
    }
}
//单击播放列表
void MyWidget::tableClicked(QModelIndex index)
{
    //首先获取媒体对象当前的状态，然后停止播放并清空播放队列
    bool wasPlaying = mediaObject->state() == QMediaPlayer::PlayingState;
    mediaObject->stop();

    //如果单击的播放列表中的行号大于媒体源列表的大小，则直接返回
    if(index.row() >= sources.size())
        return;

    //设置单击的行对应的媒体源为媒体对象的当前媒体源
    mediaObject->setMedia(sources.at(index.row()));

    //如果以前媒体对象处于播放状态，那么开始播放选中的曲目
    if(wasPlaying)
        mediaObject->play();
}
//清空媒体源列表，它与播放列表的playListClean()信号关联
void MyWidget::clearSources()
{
    sources.clear();

    //更改动作图标状态
    changeActionState();
}
//初始化播放器
void MyWidget::initPlayer()
{
    //设置主界面标题、图标和大小
    setWindowTitle(tr("MyPlayer音乐播放器"));
    setWindowIcon(QIcon(":/images/icon.png"));
    setMinimumSize(320,160);
    setMaximumSize(320,160);
    //创建媒体图
    mediaObject = new QMediaPlayer(this);

    // 关联媒体对象的tick()信号来更新播放时间的显示
    connect(mediaObject, SIGNAL(positionChanged(qint64)), this, SLOT(updateTime(qint64)));

    // 创建顶部标签，用于显示一些信息
    topLabel = new QLabel(tr("<a href = \" http://www.yafeilinux.com \"> www.yafeilinux.com </a>"));
    topLabel->setTextFormat(Qt::RichText);
    topLabel->setOpenExternalLinks(true);
    topLabel->setAlignment(Qt::AlignCenter);

    // 创建控制播放进度的滑块
     QSlider *seekSlider = new QSlider(this);
     seekSlider->setOrientation(Qt::Horizontal);
     connect(mediaObject,&QMediaPlayer::positionChanged,[=](qint64 position){
         seekSlider->setMaximum(mediaObject->duration()/1000);
         seekSlider->setValue(position/1000);
     });
     connect(seekSlider,&QSlider::sliderMoved,[=](int position){
         mediaObject->pause();
         mediaObject->setPosition(position*1000);
     });
     connect(seekSlider,&QSlider::sliderReleased,[=](){
         mediaObject->play();
     });

    // 创建包含播放列表图标、显示时间标签和桌面歌词图标的工具栏
    QToolBar *widgetBar = new QToolBar(this);
    // 显示播放时间的标签
    timeLabel = new QLabel(tr("00:00 / 00:00"), this);
    timeLabel->setToolTip(tr("当前时间 / 总时间"));
    timeLabel->setAlignment(Qt::AlignCenter);
    timeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // 创建图标，用于控制是否显示播放列表
    QAction *PLAction = new QAction(tr("PL"), this);
    PLAction->setShortcut(QKeySequence("F4"));
    PLAction->setToolTip(tr("播放列表(F4)"));
    connect(PLAction, SIGNAL(triggered()), this, SLOT(setPlaylistShown()));
    // 创建图标，用于控制是否显示桌面歌词
    QAction *LRCAction = new QAction(tr("LRC"), this);
    LRCAction->setShortcut(QKeySequence("F2"));
    LRCAction->setToolTip(tr("桌面歌词(F2)"));
    connect(LRCAction, SIGNAL(triggered()), this, SLOT(setLrcShown()));
    // 添加到工具栏
    widgetBar->addAction(PLAction);
    widgetBar->addSeparator();
    widgetBar->addWidget(timeLabel);
    widgetBar->addSeparator();
    widgetBar->addAction(LRCAction);

    // 创建播放控制动作工具栏
    QToolBar *toolBar = new QToolBar(this);
    // 播放动作
    playAction = new QAction(this);
    playAction->setIcon(QIcon(":/images/play.png"));
    playAction->setText(tr("播放(F5)"));
    playAction->setShortcut(QKeySequence(tr("F5")));
    connect(playAction, SIGNAL(triggered()), this, SLOT(setPaused()));
    // 停止动作
    stopAction = new QAction(this);
    stopAction->setIcon(QIcon(":/images/stop.png"));
    stopAction->setText(tr("停止(F6)"));
    stopAction->setShortcut(QKeySequence(tr("F6")));
    connect(stopAction, SIGNAL(triggered()), mediaObject, SLOT(stop()));
    // 跳转到上一首动作
    skipBackwardAction = new QAction(this);
    skipBackwardAction->setIcon(QIcon(":/images/skipBackward.png"));
    skipBackwardAction->setText(tr("上一首(Ctrl+Left)"));
    skipBackwardAction->setShortcut(QKeySequence(tr("Ctrl+Left")));
    connect(skipBackwardAction, SIGNAL(triggered()), this, SLOT(skipBackward()));
    // 跳转到下一首动作
    skipForwardAction = new QAction(this);
    skipForwardAction->setIcon(QIcon(":/images/skipForward.png"));
    skipForwardAction->setText(tr("下一首(Ctrl+Right)"));
    skipForwardAction->setShortcut(QKeySequence(tr("Ctrl+Right")));
    connect(skipForwardAction, SIGNAL(triggered()), this, SLOT(skipForward()));
    // 打开文件动作
    QAction *openAction = new QAction(this);
    openAction->setIcon(QIcon(":/images/open.png"));
    openAction->setText(tr("播放文件(Ctrl+O)"));
    openAction->setShortcut(QKeySequence(tr("Ctrl+O")));
    connect(openAction, SIGNAL(triggered()), this, SLOT(openFile()));
    // 音量控制部件
    QSlider *volumeSlider = new QSlider(this);
    volumeSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    volumeSlider->setValue(mediaObject->volume());
    connect(volumeSlider,SIGNAL(valueChanged(int)),mediaObject,SLOT(setVolume(int)));
    // 添加到工具栏
    toolBar->addAction(playAction);
    toolBar->addSeparator();
    toolBar->addAction(stopAction);
    toolBar->addSeparator();
    toolBar->addAction(skipBackwardAction);
    toolBar->addSeparator();
    toolBar->addAction(skipForwardAction);
    toolBar->addSeparator();
    toolBar->addWidget(volumeSlider);
    toolBar->addSeparator();
    toolBar->addAction(openAction);

    // 创建主界面布局管理器
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(topLabel);
    mainLayout->addWidget(seekSlider);
    mainLayout->addWidget(widgetBar);
    mainLayout->addWidget(toolBar);
    setLayout(mainLayout);

    //mediaObject->setMedia(QUrl::fromLocalFile("../myPlayer/music.mp3"));

    //创建播放列表
    playlist = new MyPlaylist(this);
    connect(playlist,SIGNAL(clicked(QModelIndex)),this,SLOT(tableClicked(QModelIndex)));
    connect(playlist,SIGNAL(playlistClean()),this,SLOT(clearSources()));

    //创建用来解析媒体的信息的元信息解析器
    metaInformationResolver = new QMediaPlayer();

    //需要与AudioOutput连接后才能使用metaInformationResolver来获取歌曲的总时间
    connect(metaInformationResolver,
            SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),this,
            SLOT(metaStateChanged(QMediaPlayer::MediaStatus)));

    //初始化动作图标的状态
    playAction->setEnabled(false);
    stopAction->setEnabled(false);
    skipBackwardAction->setEnabled(false);
    skipForwardAction->setEnabled(false);
    topLabel->setFocus();
}
//根据媒体源列表内容和当前媒体源的位置来改变主界面图标的状态
void MyWidget::changeActionState()
{
    //如果媒体源列表为空
    if(sources.count() == 0){
        //如果没有在播放歌曲，则播放和停止按钮都不可用
        //（因为可能歌曲正在播放时清除了播放列表）
        if(mediaObject->state() != QMediaPlayer::PlayingState
                && mediaObject->state() != QMediaPlayer::PausedState){
            playAction->setEnabled(false);
            stopAction->setEnabled(false);
        }
        skipBackwardAction->setEnabled(false);
        skipForwardAction->setEnabled(false);
    }else{//如果媒体源列表不为空
        playAction->setEnabled(true);
        stopAction->setEnabled(true);
        //如果媒体源列表只有一行
        if(sources.count() == 1){
            skipBackwardAction->setEnabled(false);
            skipForwardAction->setEnabled(false);
        }else{//如果媒体源列表有多行
            skipBackwardAction->setEnabled(true);
            skipForwardAction->setEnabled(true);
            int index = playlist->currentRow();
            //如果播放列表当前选中的行为第一行
            if(index == 0)
                skipBackwardAction->setEnabled(false);
            //如果播放列表当前选中的行为最后一行
            if(index + 1 == sources.count())
                skipForwardAction->setEnabled(false);
        }
    }
}
