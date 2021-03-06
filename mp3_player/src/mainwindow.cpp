#include "../headers/mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    player = new QMediaPlayer;
    player->setVolume(DEFAULT_VOLUME);
    playlist = std::make_unique<QMediaPlaylist>();
    actualPlaylistName = "";
    songName = "";

    //levelVolume = 100; // domyslna wartosc dzwieku

    levelVolume = DEFAULT_VOLUME; // domyslna wartosc dzwieku
    samplesCount = DEFAULT_SAMPLES;

    visualisation = new Visualisation();// utworzenie nowego okienka
    authors = new Authors();

    // stateMachine
    auto stateMachine = new QStateMachine(this);

    auto startState = new QState(stateMachine);
    auto hasPlaylistState = new QState(stateMachine);
    auto playState = new QState(stateMachine);
    auto stopState = new QState(stateMachine);
    auto pauseState = new QState(stateMachine);

    // assignProperty to states
    // startState
    startState->assignProperty(ui->pbNewPlaylist, "enabled", true);
    startState->assignProperty(ui->pbDeletePlaylist, "enabled", false);
    startState->assignProperty(ui->pbAddSong, "enabled", false);
    startState->assignProperty(ui->pbDeleteSong, "enabled", false);
    startState->assignProperty(ui->pbVisualisation, "enabled", false);
    startState->assignProperty(ui->pbNextSingiel, "enabled", false);
    startState->assignProperty(ui->pbPreviousSingiel, "enabled", false);
    startState->assignProperty(ui->pbPlayMusic, "enabled", false);
    startState->assignProperty(ui->pbStopMusic, "enabled", false);
    startState->assignProperty(ui->pbPauseMusic, "enabled", false);

    // hasPlaylistState
    hasPlaylistState->assignProperty(ui->pbAddSong, "enabled", true);
    hasPlaylistState->assignProperty(ui->pbDeletePlaylist, "enabled", true);
    hasPlaylistState->assignProperty(ui->pbDeleteSong, "enabled", false);
    hasPlaylistState->assignProperty(ui->pbVisualisation, "enabled", false);
    hasPlaylistState->assignProperty(ui->pbNextSingiel, "enabled", false);
    hasPlaylistState->assignProperty(ui->pbPreviousSingiel, "enabled", false);
    hasPlaylistState->assignProperty(ui->pbPlayMusic, "enabled", false);
    hasPlaylistState->assignProperty(ui->pbStopMusic, "enabled", false);
    hasPlaylistState->assignProperty(ui->pbPauseMusic, "enabled", false);

    // playState
    playState->assignProperty(ui->pbPlayMusic, "enabled", false);
    playState->assignProperty(ui->pbStopMusic, "enabled", true);
    playState->assignProperty(ui->pbPauseMusic, "enabled", true);
    playState->assignProperty(ui->pbNextSingiel, "enabled", true);
    playState->assignProperty(ui->pbPreviousSingiel, "enabled", true);
    playState->assignProperty(ui->pbDeleteSong, "enabled", true);
    playState->assignProperty(ui->pbVisualisation, "enabled", true);

    // stopState
    stopState->assignProperty(ui->pbPlayMusic, "enabled", true);
    stopState->assignProperty(ui->pbStopMusic, "enabled", false);
    stopState->assignProperty(ui->pbPauseMusic, "enabled", false);
    stopState->assignProperty(ui->pbNextSingiel, "enabled", false);
    stopState->assignProperty(ui->pbPreviousSingiel, "enabled", false);

    // pauseState
    pauseState->assignProperty(ui->pbStopMusic, "enabled", true);
    pauseState->assignProperty(ui->pbPlayMusic, "enabled", true);
    pauseState->assignProperty(ui->pbPauseMusic, "enabled", false);
    pauseState->assignProperty(ui->pbNextSingiel, "enabled", false);
    pauseState->assignProperty(ui->pbPreviousSingiel, "enabled", false);


    //////////////////////////////////////////////////////////////////


    // transitions + connects
    startState->addTransition(this, SIGNAL(sigHasPlaylist()), hasPlaylistState);

    connect(hasPlaylistState, SIGNAL(entered()), this, SLOT(slotHasSong()));

    hasPlaylistState->addTransition(this,SIGNAL(sigNoPlaylist()),startState);
    hasPlaylistState->addTransition(this,SIGNAL(sigPlaySong()),playState);

    connect(playState, SIGNAL(entered()), SLOT(slotPlay()));

    playState->addTransition(ui->pbStopMusic, SIGNAL(clicked()), stopState);
    playState->addTransition(ui->pbPauseMusic, SIGNAL(clicked()), pauseState);
    playState->addTransition(this, SIGNAL(sigNoSong()), hasPlaylistState);
    playState->addTransition(this, SIGNAL(sigNoPlaylist()),startState);

    connect(stopState, SIGNAL(entered()), SLOT(slotStopMusic()));

    stopState->addTransition(ui->pbPlayMusic, SIGNAL(clicked()), playState);
    stopState->addTransition(this, SIGNAL(sigPlaySong()), playState);
    stopState->addTransition(this, SIGNAL(sigNoSong()), hasPlaylistState);
    stopState->addTransition(this,SIGNAL(sigNoPlaylist()),startState);

    pauseState->addTransition(ui->pbPlayMusic, SIGNAL(clicked()), playState);
    pauseState->addTransition(ui->pbStopMusic, SIGNAL(clicked()), stopState);
    pauseState->addTransition(this, SIGNAL(sigPlaySong()), playState);
    pauseState->addTransition(this, SIGNAL(sigNoSong()), hasPlaylistState);
    pauseState->addTransition(this,SIGNAL(sigNoPlaylist()),startState);

    // progress bar, nie pytajcie czemu tak, inaczej nie działa
    connect( player, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged );        // gdy zmienia się pozycja w piosence to player wysyła sygnał positionChanged
    connect( player, &QMediaPlayer::durationChanged, this, &MainWindow::onDurationChanged );        // jak player wczyta z pliku piosenkę to ustawia swój parametr duration i wysyła sygnał durationChanged

    connect(this, SIGNAL(sigSongChange()), SLOT(slotPlay()));
    connect(playlist.get(), &QMediaPlaylist::currentMediaChanged, this, &MainWindow::on_currentMediaChanged);
    connect(ui->pbAddSong, SIGNAL(clicked()), SLOT(slotAddSong()));

    // start machine
    stateMachine->setInitialState(startState);
    stateMachine->start();

    if(hasPlaylistsToLoad()){
        ui->listPlaylists->setCurrentRow(0);
        actualPlaylistName = ui->listPlaylists->currentItem()->text();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::hasPlaylistsToLoad()
{
    QString playlistsPath = QDir::currentPath().append("/playlists/");
    QDirIterator iterator(playlistsPath,QDir::Files);
    bool hasPlaylist = false;
    while (iterator.hasNext()) {
        QFileInfo playlistFile(iterator.next());
        QString playlistName = playlistFile.baseName();

        ui->listPlaylists->addItem(playlistName);
        hasPlaylist = true;
    }
    return hasPlaylist;
}

void MainWindow::slotPlay()
{
      player->play();
}

void MainWindow::slotHasSong()
{
    if(ui->listSongs->count() != 0 ){
        emit sigPlaySong();
    }
}

void MainWindow::slotStopMusic()
{
    player->stop();
    emit sigStopMusic();
}

void MainWindow::slotAddSong()
{
    auto playlistName = ui->listPlaylists->currentItem()->text();
    actualPlaylistName = playlistName;

    auto fileNames = QFileDialog::getOpenFileNames(this, tr("Choose songs"),
                                                   QDir::currentPath(),
                                                   tr("MP3 files (*.mp3)"));

    bool flagHasSong = false;
    for(auto fileName: fileNames) {
        playlist->addMedia(QMediaContent(QUrl::fromLocalFile(fileName)));
        ui->listSongs->addItem(QUrl(fileName).fileName());
        flagHasSong = true;
    }

    QString filePath = QDir::currentPath().append("/playlists/"+playlistName+".m3u");

    if(playlist->save(QUrl::fromLocalFile(filePath),"m3u")){
        qDebug() << "Playlist saved succesfully - slotAddSong";
    } else {
        qDebug() << "Playlist not saved - - slotAddSong";
    }
    if(flagHasSong) {
        ui->listSongs->setCurrentRow(0);
        emit sigPlaySong();
        emit sigSongChange();
    }
}


void MainWindow::on_sliderLevelVolume_valueChanged(int value)
{
    levelVolume = value;
    player->setVolume(levelVolume);
}

void MainWindow::on_sliderProgress_sliderMoved(int position)
{
    player->setPosition(position);
}

void MainWindow::onPositionChanged(int position)
{
    ui->sliderProgress->setValue(position);
    curTime.min = position/(1000*60);            // pozycja przez 1000*60 milisekund = 60 sekund
    curTime.sec = (position/1000)%60;            // pozycja przez 1000 milisekund daje czyste sekundy, modulo 60 resetuje sekundy gdy dobiją do 59
    QString s;
    s.sprintf("%2d:%02d", curTime.min, curTime.sec);
    ui->labelProgress->setText(s);
}

void MainWindow::onDurationChanged(int duration)
{
    ui->sliderProgress->setMaximum(duration);
}

void MainWindow::on_pbPauseMusic_clicked()
{
    player->pause();
}

void MainWindow::on_pbNextSingiel_clicked()
{
    int temp = playlist->currentIndex();
    playlist->next();

    while ( temp == playlist->currentIndex() )
        playlist->next();

    auto currentIndex = playlist->currentIndex();
    if(currentIndex == -1)
        currentIndex = 0;

    ui->listSongs->setCurrentRow(currentIndex);
    emit sigSongChange();
}

void MainWindow::on_pbPreviousSingiel_clicked()
{
    playlist->previous();
    auto currentIndex = playlist->currentIndex();

    if(currentIndex == -1)
        currentIndex = 0;

    ui->listSongs->setCurrentRow(currentIndex);
    emit sigSongChange();
}

void MainWindow::on_pbDeleteSong_clicked()
{
    auto songToDelete = ui->listSongs->currentItem();
    QMessageBox msgBox;
    msgBox.setWindowTitle("Delete song");
    msgBox.setText("Do you want to delete this song?");
    msgBox.setInformativeText(songToDelete->text());
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);
    int ret = msgBox.exec();

    switch (ret) {
      case QMessageBox::Ok:
        {
          auto rowToDelete = ui->listSongs->row(songToDelete);
          playlist->removeMedia(rowToDelete);
          ui->listSongs->removeItemWidget(songToDelete);
          auto it = ui->listSongs->takeItem(rowToDelete);
          savePlaylist(*playlist);

          if(ui->listSongs->count() == 0) { emit sigNoSong(); }

          delete it;
          break;
        }
      case QMessageBox::Cancel:
          // Cancel was clicked
          break;

    }
}


void MainWindow::on_pbNewPlaylist_clicked()
{
    std::unique_ptr<QInputDialog> addPlaylistDialog = std::make_unique<QInputDialog>();
    addPlaylistDialog->resize(300,300);
    addPlaylistDialog->setInputMode(QInputDialog::TextInput);
    addPlaylistDialog->setWindowTitle("Create Playlist");
    addPlaylistDialog->setOkButtonText("Choose songs");
    addPlaylistDialog->setLabelText("Name your playlist");
    addPlaylistDialog->setTextValue("new_playlist");

    auto ok = addPlaylistDialog->exec();
    auto playlistName = addPlaylistDialog->textValue();

    if (ok && !playlistName.isEmpty()) {
        auto listPlaylistNames = ui->listPlaylists->findItems(playlistName,Qt::MatchExactly);
        if(!listPlaylistNames.isEmpty()) {
            QMessageBox::information(this,tr("Playlist warning"),
                                 tr("This name is already taken.\nRename it."),
                                 QMessageBox::Cancel);
        } else {
            playlist->clear();
            ui->listSongs->clear();
            ui->listPlaylists->addItem(playlistName);
            auto fileNames = QFileDialog::getOpenFileNames(this, tr("Choose songs"),
                                                          QDir::currentPath(),
                                                          tr("MP3 files (*.mp3)"));
            for(auto fileName: fileNames) {
                playlist->addMedia(QMediaContent(QUrl::fromLocalFile(fileName)));
                ui->listSongs->addItem(QUrl(fileName).fileName());
            }
            player->setPlaylist(playlist.get());

            QDir playlistDir("");
            if(playlistDir.mkdir("playlists")) {
                qDebug() << "Folder playlists created in build";
            } else {
                qDebug() << "Folder playlists already exists or ERROR";
            }

            QString filePath = QDir::currentPath().append("/playlists/"+playlistName+".m3u");

            if(playlist->save(QUrl::fromLocalFile(filePath),"m3u")){
                qDebug() << "Playlist saved succesfully pbNewPlaylist clicked";
                emit sigHasPlaylist();
                emit sigSongChange();
            } else {
                qDebug() << "Playlist not saved - pbNewPlaylist clicked";
            }
            auto playlistRow = ui->listPlaylists->count()-1;
            ui->listPlaylists->setCurrentRow(playlistRow);
            ui->listSongs->setCurrentRow(0);
            actualPlaylistName = ui->listPlaylists->currentItem()->text();
        }
    }
}

void MainWindow::on_pbDeletePlaylist_clicked()
{
    auto playlistItem = ui->listPlaylists->currentItem();
    auto playlistRow = ui->listPlaylists->currentRow();
    auto playlistName = playlistItem->text();
    QString filePath = QDir::currentPath().append("/playlists/"+playlistName+".m3u");
    playlist->clear();
    QFile playlistFile(filePath);
    playlistFile.remove();
    ui->listSongs->clear();
    ui->listPlaylists->removeItemWidget(playlistItem);
    auto it = ui->listPlaylists->takeItem(playlistRow);
    delete it;

    if(ui->listPlaylists->count()) {
        ui->listPlaylists->setCurrentRow(0);
    } else {
        ui->listPlaylists->clear();
        ui->labelNowPlaySong->clear();
        emit sigNoPlaylist();
    }
}

void MainWindow::on_listSongs_itemDoubleClicked()
{
    auto currentRow = ui->listSongs->currentRow();
    playlist->setCurrentIndex(currentRow);
    emit sigPlaySong();
}

void MainWindow::on_listPlaylists_itemDoubleClicked()
{
    ui->listSongs->clear();
    auto playlistName = ui->listPlaylists->currentItem()->text();
    actualPlaylistName = playlistName;
    QString filePath = QDir::currentPath().append("/playlists/"+playlistName+".m3u");
    playlist->clear();
    playlist->load(QUrl::fromLocalFile(filePath),"m3u");
    player->setPlaylist(playlist.get());

    QFile fileNames(filePath);
    fileNames.open(QIODevice::ReadOnly | QIODevice::Text); //zrobic errora

    ui->listSongs->clear();
    while (!fileNames.atEnd()) {
        auto fileName = QString(fileNames.readLine());
        fileName.remove("\n");
        fileName.remove("file://");

        ui->listSongs->addItem(QUrl(fileName).fileName());
    }
    ui->listSongs->setCurrentRow(0);
    emit sigHasPlaylist();

    if( ui->listSongs->count() != 0) {
        emit sigPlaySong();
        emit sigSongChange();
    } else {
        emit sigNoSong();
    }
}

void MainWindow::on_currentMediaChanged(const QMediaContent &content)
{
    auto songTitle = content.canonicalUrl().fileName();
    ui->labelNowPlaySong->setText(songTitle);
}

void MainWindow::on_radioButton_clicked(bool checked)
{
    if ( checked )
        playlist->setPlaybackMode(QMediaPlaylist::Random);

    else
        playlist->setPlaybackMode(QMediaPlaylist::Loop);
}

void MainWindow::savePlaylist(QMediaPlaylist &playlist)
{
    qDebug() << actualPlaylistName;
    QString playlistPath = QDir::currentPath().append("/playlists/"+actualPlaylistName+".m3u");
    if(playlist.save(QUrl::fromLocalFile(playlistPath),"m3u")){
        qDebug() << "Playlist saved succesfully";
    } else {
        qDebug() << "Playlist not saved";
    }
}


void MainWindow::on_pbVisualisation_clicked(bool checked)
{
    if ( !checked )
    {
        visualisation->show();                      // wyświetlenie tego okienka
        probe = new QAudioProbe();
        probe->setSource(player);                   // ustawienie, aby sonda "śledziła" to co odtwarza player
        inputArray = new double[MAX_SAMPLES];       // tablica wejściowa będzie przechowywać maksymalnie 2^14 (16834) próbek
        curSize = 0;                                // początkowa ilość elementów w tablicy z próbkami
        connect( probe, SIGNAL(audioBufferProbed(QAudioBuffer)), this, SLOT(processBuffer(QAudioBuffer)) );
        connect( visualisation, SIGNAL( shift()), this, SLOT(shift()) );
        connect( visualisation, SIGNAL( noshift()), this, SLOT(noshift()) );
        connect( visualisation, SIGNAL( samplesCountChanged(int)), this, SLOT(arraySizeChanged(int)) );
    }
}

void MainWindow::processBuffer(QAudioBuffer buffer)
{
    visualisation->setSamplesPerSecond( buffer.format().sampleRate() * buffer.format().channelCount() );
    // w buforze zawsze są 2304 próbki (przynajmniej na moim komputerze, sprawdzone empirycznie, nie wiem od czego to zależy)
    const qint16* data = buffer.constData<qint16>();        // próbki są w postaci short intów
    bufferSize = buffer.sampleCount();

    if( curSize <= samplesCount )
    {
        dataIndex = 0;
        while( curSize <= samplesCount && dataIndex < bufferSize )
            inputArray[curSize++] = (double)data[dataIndex++]/SHRT_MAX;
    }
    else
    {
        SignalPower signalPower(curSize, inputArray);
        visualisation->displayDB(signalPower.getPower());

        FastFourier fft(samplesCount, inputArray);
        fft.Calculate();
        if( shifted )
            fft.fftshift();     // opcjonalnie fftshift jak w MatLABie
        visualisation->prepareData( samplesCount, fft.getResult() );     // wysyła do okienka visualisation ilość próbek i tablicę próbek transformaty
        curSize = 0;

        while( curSize <= samplesCount && dataIndex < bufferSize )             // przepisanie reszty próbek z bufora piosenki do inputArray
            inputArray[curSize++] = (double)data[dataIndex++]/SHRT_MAX;
    }
}


void MainWindow::shift()
{
    shifted =  true;
}

void MainWindow::noshift()
{
    shifted = false;
}

void MainWindow::arraySizeChanged(int samplesCount)
{
    this->samplesCount = samplesCount;
}

void MainWindow::on_pbAuthors_clicked()
{
    authors->show();
}
