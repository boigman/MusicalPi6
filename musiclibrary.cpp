// Copyright 2023 by Linwood Ferguson, licensed under GNU GPLv3

#include <QPainter>
#include <QSqlRecord>
#include <QSqlField>
#include <QSqlError>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QCheckBox>

#include <QHeaderView>
#include <QScroller>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QEvent>
#include <QKeyEvent>
#include <QTableWidgetItem>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QListView>

#include "musiclibrary.h"
#include "mainwindow.h"
#include "oursettings.h"
#include "piconstants.h"

#include <cassert>


musicLibrary::musicLibrary(QWidget *parent, MainWindow* mp) : QWidget(parent)
{
    // Isolate specific librarydetails in this class so we could switch to different
    // calling process, also isolate formatting and population of the widget here.

    // This widget (i.e. "this") is persistent and created once. Some items are persisted
    // and available publically (e.g. the dropdown, which implicitly holds all playlists).
    // The playLists class uses these, isolating the actual database part to this routine
    // to hopefully make it easier if a different library database is used.

    ourParent = parent;
    mParent = mp;
    calibreMusicTag   =  mParent->ourSettingsPtr->getSetting("calibreMusicTag").toString();
    calibrePath       =  mParent->ourSettingsPtr->getSetting("calibrePath").toString();
    calibreDatabase   =  mParent->ourSettingsPtr->getSetting("calibreDatabase").toString();
    calibreListPrefix = mParent->ourSettingsPtr->getSetting("calibreListPrefix").toString();
    forceOnboardKeyboard = mParent->ourSettingsPtr->getSetting("forceOnboardKeyboard").toBool();

    ActiveListIndex = 0;  // Default for start -- choices are persisted against subsequent hide/show iterations.

    m_db = NULL;  // Be sure we know if it's been created.  We open/close it a lot, but do not delete it until this routine is deleted

    qDebug() << "in constructor with path " << calibrePath << " and file " << calibreDatabase;

    // Create widgets and layouts

    baseLayout = new QVBoxLayout(this);
    search = new QWidget(this);
    searchLayout = new QHBoxLayout(search);
    prompt = new QLabel(search);
    searchBox = new QLineEdit(search);
    dropdown= new QComboBox(search);
    listDropdown = new QListView(dropdown);
    dropdown->setView(listDropdown);
    libTable = new QTableWidget(this);
    checkboxLabel = new QLabel(this);
    checkboxAll = new QCheckBox(this);
    checkboxNone = new QCheckBox(this);
    checkbox1 = new QCheckBox(this);
    checkbox2 = new QCheckBox(this);
    checkbox3 = new QCheckBox(this);
    checkbox4 = new QCheckBox(this);

    // Arrange the widgets

    this->setLayout(baseLayout);
    baseLayout->addWidget(search);
    baseLayout->addWidget(libTable);
    search->setLayout(searchLayout);
    searchLayout->addWidget(prompt);
    searchLayout->addWidget(searchBox);
    searchLayout->addWidget(dropdown);
    searchLayout->addWidget(checkboxLabel);
    searchLayout->addWidget(checkboxAll);
    searchLayout->addWidget(checkboxNone);
    searchLayout->addWidget(checkbox1);
    searchLayout->addWidget(checkbox2);
    searchLayout->addWidget(checkbox3);
    searchLayout->addWidget(checkbox4);
    checkboxLabel->setFont(QFont("Arial",12,1));
    checkboxLabel->setText("  Additional Filter(s):  ");
    checkboxAll->setText("All");
    checkboxNone->setText("None");
    checkbox1->setText("CheckBox1");
    checkbox2->setText("CheckBox2");
    checkbox3->setText("CheckBox3");
    checkbox4->setText("CheckBox4");
    checkbox1->setVisible(false);
    checkbox1->setVisible(false);
    checkbox1->setVisible(false);
    checkbox1->setVisible(false);
    checkboxAll->setVisible(false);



    prompt->setFont(QFont("Arial",16,1));
    prompt->setText("Search for:");
    searchBox->setMinimumWidth(300);  // Looks dumb if too short, but also if too long
    searchBox->setMaximumWidth(300);
    searchBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    searchBox->setAlignment(Qt::AlignLeft);
    searchLayout->setAlignment(Qt::AlignLeft);
    searchBox->installEventFilter(search);  // So we can catch keystrokes and do as-you-type filter

    libTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    libTable->setSelectionMode(QAbstractItemView::SingleSelection);
    libTable->verticalHeader()->hide();
    libTable->setAlternatingRowColors(true);
    libTable->setSortingEnabled(true);
    libTable->setEditTriggers(QAbstractItemView::NoEditTriggers);  // this makes the table read-only so you don't accidentally end up editing inside a cell.

    QScroller::grabGesture(libTable,QScroller::LeftMouseButtonGesture);  // so a touch-drag will work, otherwise need two finger drag
    libTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    libTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    libTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(libTable, SIGNAL(cellClicked(int,int)), this, SLOT(onChosen(int,int)));
    connect(searchBox, SIGNAL(textChanged(QString)), this, SLOT(filterTable(QString)));
    connect(checkboxAll, SIGNAL(stateChanged(int)), this, SLOT(procCboxAll(int)));
    connect(checkboxNone, SIGNAL(stateChanged(int)), this, SLOT(procCboxNone(int)));
    connect(checkbox1, SIGNAL(stateChanged(int)), this, SLOT(procCbox1(int)));
    connect(checkbox2, SIGNAL(stateChanged(int)), this, SLOT(procCbox2(int)));
    connect(checkbox3, SIGNAL(stateChanged(int)), this, SLOT(procCbox3(int)));
    connect(checkbox4, SIGNAL(stateChanged(int)), this, SLOT(procCbox4(int)));

    m_db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
    m_db->setDatabaseName(calibrePath + "/" + calibreDatabase);
    m_db->open();
    QSqlQuery queryLists;
    QString sql = "SELECT t.name as tag_name, count(1) as count FROM tags t, books_tags_link btl where t.id=btl.tag and t.name<>'"
            + calibreMusicTag + "' and t.name not like '" + calibreListPrefix + "%' group by t.name order by count(1) desc;";
    queryLists.exec(sql);
    checkSqlError("Executed tag query " + sql, queryLists.lastError());
    int iQcount = 0;
    while(queryLists.next())
    {
        iQcount++;
        qDebug()<<"read tags, value=" << queryLists.record().field(0).value().toString();
        if(iQcount==1) {
            checkbox1->setText(queryLists.record().field(0).value().toString());
            checkbox1->setVisible(true);
        } else if(iQcount==2) {
            checkbox2->setText(queryLists.record().field(0).value().toString());
            checkbox2->setVisible(true);
        } else if(iQcount==3) {
            checkbox3->setText(queryLists.record().field(0).value().toString());
            checkbox3->setVisible(true);
        } else if(iQcount==4) {
            checkbox4->setText(queryLists.record().field(0).value().toString());
            checkbox4->setVisible(true);
        }
    }
    m_db->close();
    checkboxNone->setChecked(true);
//    screenLoaded=false;
//    lastTimeSelected = QTime::currentTime();
}

musicLibrary::~musicLibrary()
{
    qDebug() << "in destructor";
    DELETE_LOG(m_db);
}

void musicLibrary::loadPlayLists()
{
    // Get a list of playlists and set it to active if we already had chosen it
    // Must have database open to call
    // These need to stay loaded as they are used in a separate widget (and indeed may be forced into a reload by it)

    qDebug() << "Entered";
    dropdown->clear();
    QSqlQuery queryLists;
//    QString sql = "select id, name from tags where name like '" + calibreListPrefix + "%' order by name;";
    QString sql = "select t.id, case instr(t.name,'|') when 0 then t.name else substr(t.name, 1, instr(t.name,'|')-1) end as tagname from tags t"
            ", books_tags_link btl where t.id=btl.tag and t.name like '" + calibreListPrefix + "%' order by t.name;";
    queryLists.exec(sql);
    checkSqlError("Executed tag query " + sql, queryLists.lastError());
    dropdown->addItem("All items",0);
    QString lastPlaylist="none";
    while(queryLists.next())
    {
        QString playlist = queryLists.record().field(1).value().toString().replace(calibreListPrefix,"");
        qDebug()<<"read tags, id=" << queryLists.record().field(0).value().toInt() << ", value=" << queryLists.record().field(1).value().toString();
        if(playlist!=lastPlaylist) {
            dropdown->addItem(playlist.replace(calibreListPrefix,""),queryLists.record().field(0).value().toInt());
        } else {
            int tmpIndex = dropdown->findText(playlist);
            dropdown->removeItem(tmpIndex);
            dropdown->addItem(playlist.replace(calibreListPrefix,""),queryLists.record().field(0).value().toInt());
        }
        lastPlaylist = playlist;
    }
    ActiveListIndex = dropdown->findText(ActiveList);  // We have to look it up in case they list changed so index may change
    if(ActiveListIndex == -1) ActiveListIndex = 0;  // If the current active disappeareed reset to all
    dropdown->setCurrentIndex(ActiveListIndex);
}

void musicLibrary::loadBooks()
{
    // Must have database open to call
    qDebug() << "Entered";
    lastRowSelected=-1; // None selected (yet)
    libTable->setRowCount(0);
    searchBox->setText("");  // Start fresh search each time
    QSqlQuery queryBooks;
    QString sql =
        "select b.id as BookID, b.sort as Title, coalesce(max(s.name),'') as Collection, b.author_sort as Author, b.path || '/' ||  d.name || '.' || lower(d.format) as Path, group_concat(t2.name,',') as tags "
        "from books b "
        "inner join books_tags_link btl on btl.book = b.id "
        "inner join tags t on t.id = btl.tag "
        "inner join data d on d.book = b.id and d.format = 'PDF' " +
        (
           ActiveListIndex == 0 ? "" :
              ( "inner join books_tags_link btl3 on btl3.book = b.id "
                "inner join tags t3 on t3.id = btl3.tag and t3.name like '" + calibreListPrefix + ActiveList + "%' "
              )
        ) +
        "left join books_series_link bsl on bsl.book = b.id "
        "left join series s on s.id=bsl.series "
        "left join books_tags_link btl2 on btl2.book=b.id "
        "left join tags t2 on t2.id=btl2.tag and t2.name not like 'musicList%' and t2.name <> 'music' " +
            (
               !checkbox1->isChecked() ? "" :
                  ( "inner join books_tags_link btl3 on btl3.book = b.id "
                    "inner join tags t3 on t3.id = btl3.tag and t3.name = '" + checkbox1->text() + "' "
                  )
            ) +
           (
              !checkbox2->isChecked() ? "" :
                 ( "inner join books_tags_link btl4 on btl4.book = b.id "
                   "inner join tags t4 on t4.id = btl4.tag and t4.name = '" + checkbox2->text() + "' "
                 )
           ) +
           (
              !checkbox3->isChecked() ? "" :
                 ( "inner join books_tags_link btl5 on btl5.book = b.id "
                   "inner join tags t5 on t5.id = btl5.tag and t5.name = '" + checkbox3->text() + "' "
                 )
           ) +
           (
              !checkbox4->isChecked() ? "" :
                 ( "inner join books_tags_link btl6 on btl6.book = b.id "
                   "inner join tags t6 on t6.id = btl6.tag and t6.name = '" + checkbox4->text() + "' "
                 )
           ) +
        "where t.name = '" + calibreMusicTag + "' "
        "group  by b.id, b.sort, b.author_sort, b.path, d.name "
//        "order by b.sort;";
        "order by " +
        (
            ActiveListIndex == 0 ? "b.sort;" :"substr(tags,instr(tags,'" + calibreListPrefix + ActiveList + "'));"
        );
    queryBooks.exec(sql);
    checkSqlError("Executed book query " + sql, queryBooks.lastError());
    QSqlRecord rec = queryBooks.record();
    libTable->setFont(QFont("Arial",10,0,false));
    libTable->setColumnCount(rec.count());
    // First run through the columns and code appropriately in the table, hiding some fields, remembering position
    for(int field = 0; field < rec.count(); field++)
    {
        libTable->setHorizontalHeaderItem(field, new QTableWidgetItem(rec.fieldName(field)));
        if(rec.fieldName(field)=="BookID")
        {
            libTable->setColumnHidden(field,true);
            columnForID=field;
        }
        if(rec.fieldName(field)=="Path")  // Remember where we put this
        {
            libTable->setColumnHidden(field,true);
            columnForPath=field;
        }
        if(rec.fieldName(field)=="Title")
        {
            columnForTitle=field;  // And remember this one
        }
    }
    int row=0;
    while(queryBooks.next())
    {
        libTable->setRowCount(row+1);
        for(int field=0; field < rec.count(); field++)
        {
            libTable->setItem(row,field,new QTableWidgetItem(queryBooks.record().field(field).value().toString()));
            if(rec.fieldName(field)=="Title") libTable->item(row,field)->setFont(QFont("Arial",14,1,false)); // Do something to make this proportional to some constant???
        }
        row++;
    }
    libTable->resizeColumnsToContents();
    qDebug() << "Completed by book retrieval and build of table";
}

void musicLibrary::showEvent(QShowEvent *e)
{
    qDebug() << "Entered";
    (void)e;
    if(!screenLoaded) // don't redo unless we need to
    {
        m_db->open();
        checkSqlError("Opening SQL database " + m_db->databaseName(), m_db->lastError());
        loadPlayLists();
        loadBooks();
        m_db->close();
    }
    connect(dropdown,SIGNAL(currentIndexChanged(int)),this,SLOT(changeList(int)));
    searchBox->installEventFilter(this);  // So we can catch keystrokes and do as-you-type filter
    searchBox->setText("");
    screenLoaded=true;
}

void musicLibrary::hideEvent(QHideEvent *e)
{
    // Note we have just hidden it, the contents stays the same.
    qDebug() << "Entered";
    (void)e;
    disconnect(dropdown,SIGNAL(currentIndexChanged(int)),this,SLOT(changeList(int)));     // Need this so we don't signal when we reload it or use it from other routines
    searchBox->removeEventFilter(this);  // So we can catch keystrokes and do as-you-type filter
}

void musicLibrary::changeList(int newListIndex)
{
    qDebug() << "Entered";
    ActiveListIndex = newListIndex;
    ActiveList = dropdown->itemText(ActiveListIndex);
    qDebug() << "Entered with " << newListIndex << " which is " << ActiveList;
    m_db->open();
    checkSqlError("Opening SQL database " + m_db->databaseName(), m_db->lastError());
    loadBooks();
    m_db->close();
}

void musicLibrary::checkSqlError(QString stage, QSqlError err) // Aborts on error
{
    if(err.databaseText()!="" || err.driverText()!="")
    {
        qDebug() << stage << " resulted in error " << err;
        assert(err.databaseText()=="" || err.driverText()=="");
    }
}

void musicLibrary::onChosen(int row, int column)
{
    QTime nowTime = QTime::currentTime();
    qDebug() << "Entered";

    if( (lastRowSelected == row) && (lastTimeSelected.secsTo(nowTime) <= 2 ) )
    {
        qDebug() << "doubleclicked on row " << row <<  " which looks like a duplicate within 2 seconds so ignoring.";
        return;
    }
    lastTimeSelected = nowTime;
    // Move these to local items since the following are going to disappear
    pathSelected = calibrePath + "/" + libTable->item(row,columnForPath)->text();
    titleSelected = libTable->item(row,columnForTitle)->text();
    qDebug() << "doubleclicked on row " << row <<  " column " << column << ", value=" << pathSelected;
    lastRowSelected = row;
    bookIDSelected = libTable->item(row,columnForID)->text().toInt();
    emit songSelected(pathSelected, titleSelected);
}


void musicLibrary::procCboxAll(int checkState)
{
    if (checkState == Qt::Checked) {
        checkboxNone->setChecked(false);
        checkbox1->setChecked(true);
        checkbox2->setChecked(true);
        checkbox3->setChecked(true);
        checkbox4->setChecked(true);
    }
    changeList(0);
}

void musicLibrary::procCboxNone(int checkState)
{
    if (checkState == Qt::Checked) {
        checkboxAll->setChecked(false);
        checkbox1->setChecked(false);
        checkbox2->setChecked(false);
        checkbox3->setChecked(false);
        checkbox4->setChecked(false);
    }
    changeList(0);
}

void musicLibrary::procCbox1(int checkState)
{
    if (checkState == Qt::Checked) {
        checkboxNone->setChecked(false);
    }
    changeList(0);
}

void musicLibrary::procCbox2(int checkState)
{
    if (checkState == Qt::Checked) {
        checkboxNone->setChecked(false);
    }
    changeList(0);
}

void musicLibrary::procCbox3(int checkState)
{
    if (checkState == Qt::Checked) {
        checkboxNone->setChecked(false);
    }
    changeList(0);
}

void musicLibrary::procCbox4(int checkState)
{
    if (checkState == Qt::Checked) {
        checkboxNone->setChecked(false);
    }
    changeList(0);
}



void musicLibrary::filterTable(QString filter)
{
    bool match;
    qDebug() << tr("Filtering for string '%1'").arg(filter);
    libTable->setUpdatesEnabled(false);
    for(int row = 0; row < libTable->rowCount(); row++)
    {
        if(filter=="")  match=true; // With no filter show everything (but go through loop in case it was previously hidden)
        else
        {  // see if any not-hidden field matches and show
            match = false;
            for(int col = 0; col < libTable->columnCount(); col++)
                if (columnForPath != col && columnForID != col &&  libTable->item(row,col)->text().contains(filter,Qt::CaseInsensitive))
                {
                    match = true;
                    break;
                }
        }
        if(match) libTable->showRow(row);
        else libTable->hideRow(row);
    }
    libTable->setUpdatesEnabled(true);
}

void musicLibrary::showKeyboard()
{
    if(!forceOnboardKeyboard) return;
    qDebug() << "Firing show keyboard";
    QDBusMessage show = QDBusMessage::createMethodCall("org.onboard.Onboard","/org/onboard/Onboard/Keyboard","org.onboard.Onboard.Keyboard","Show");
    QDBusConnection::sessionBus().send(show);
}

void musicLibrary::hideKeyboard()
{
    if(!forceOnboardKeyboard) return;
    qDebug() << "Firing hide keyboard";
    QDBusMessage show = QDBusMessage::createMethodCall("org.onboard.Onboard","/org/onboard/Onboard/Keyboard","org.onboard.Onboard.Keyboard","Hide");
    QDBusConnection::sessionBus().send(show);
}

bool musicLibrary::eventFilter(QObject *object, QEvent *event)
{
    // This is a kludge because Onboard does not work in auto-hide mode with QT, it flashes in and out
    // This forces it in when the seaerch is active, and forces it out when not.
    if(event->type() == QEvent::FocusIn && object == searchBox) showKeyboard();
    else if(event->type() == QEvent::FocusOut && object == searchBox) hideKeyboard();
    return false;
}


void musicLibrary::keyPressEvent(QKeyEvent *e)
{
    qDebug() << "Entered musicLibrary::keyPressEvent, e-Key()=" << e->key();
    if(e->key()==Qt::Key_Enter || e->key()==Qt::Key_Return) {
    int row = libTable->currentRow();
    int column = libTable->currentColumn();
        emit libTable->cellClicked(row, column);
    } else {
        if( e->key()==Qt::Key_Backspace) {
            QString textString = searchBox->text().left(searchBox->text().size()-1);
//            textString.replace(textString.length(),1,"");
           searchBox->setText(textString);
           return;
        }
        qDebug() << "Received " << QKeySequence(e->key()).toString();
//        searchBox->moveCursor (QTextCursor::End);
        if( e->key()==Qt::Key_Space) {
            searchBox->insert(" ");
        } else {
          searchBox->insert(QKeySequence(e->key()).toString());
        }
        return;
    }
}


void musicLibrary::paintEvent(QPaintEvent *) // Allows style sheets to apply in some way
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

bool musicLibrary::bookInList(int tag)
{
    m_db->open();
    checkSqlError("Opening SQL database " + m_db->databaseName(), m_db->lastError());
    QSqlQuery queryList;
    QString sql =
        "select count(*) as Presence "
        "from books_tags_link btl "
        "where btl.book = " + QString::number(bookIDSelected) + " and btl.tag = " + QString::number(tag) + ";";
    queryList.exec(sql);
    checkSqlError("Executed book/tag query " + sql, queryList.lastError());
    assert(queryList.next());
    bool ret = queryList.record().field(0).value().toBool();
    m_db->close();
    qDebug() << "Checked tag id " << tag << " for book " << bookIDSelected << " and returning " << ret;
    return ret;
}

QString musicLibrary::addBookToList(int index)
{
    // This aborts if we can't open the database but returns other errors back to the caller (or blank for none)
    qDebug() << "Request to add book " << bookIDSelected << " to tag id " << dropdown->itemData(index).toInt() << " which is " << dropdown->itemText(index);
    m_db->open();
    checkSqlError("Opening SQL database " + m_db->databaseName(), m_db->lastError());
    QSqlQuery insert;
    QString sql;
    QSqlQuery query;
    QString qsql = "select t.name from tags t where t.name like '" + calibreListPrefix + dropdown->itemText(index) + "%' order by t.name desc;";
    query.exec(qsql);
    QString qresult = query.lastError().databaseText() + " / " + query.lastError().driverText();
    bool qfound = false;
    QString qname;
    QString result;
    if(qresult == " / ")
    {
//        qDebug() << "musicLibrary::addNewList Inserting new name";
        while(query.next()) {
            qfound = true;
            qname = query.record().field(0).value().toString();
            break;
        }
        if(qfound) {
            // Got value "playlistBarnDance|01"
            int ordinal = qname.split("|")[1].toInt();
            ordinal++;
            QString newName = calibreListPrefix + dropdown->itemText(index) + "|" + QString::number(ordinal).rightJustified(2, '0');
            qDebug() << "musicLibrary::addNewList newName = " +  newName;
            sql = "insert into tags (name) values('" + newName + "')";
            insert.exec(sql);
            result = insert.lastError().databaseText() + " / " + insert.lastError().driverText();
            qDebug() << "musicLibrary::addNewList result = " +  result;
        }

    }
    qint32 rowid = insert.lastInsertId().toInt();
    sql = "insert into books_tags_link(tag, book) values (" + QString::number(rowid) + "," + QString::number(bookIDSelected) +  ");";
    insert.exec(sql);
    result = insert.lastError().databaseText() + " / " + insert.lastError().driverText();
    m_db->close();
    return (result == " / " ? "" : result);
}
QString musicLibrary::removeBookFromList(int index)
{
    // This aborts if we can't open the database but returns other errors back to the caller (or blank for none)
    qDebug() << "Request to remove book " << bookIDSelected << " to tag id " << dropdown->itemData(index).toInt() << " which is " << dropdown->itemText(index);
    m_db->open();
    checkSqlError("Opening SQL database " + m_db->databaseName(), m_db->lastError());
    QSqlQuery query;
    QString qsql = "SELECT btl.tag, t.name as tag_name, count(1) as count FROM tags t, books_tags_link btl where t.id=btl.tag and t.name like '"
            + calibreListPrefix + dropdown->itemText(index) + "%' and book = " + QString::number(bookIDSelected) + " order by t.name desc;";
    query.exec(qsql);
    QString qresult = query.lastError().databaseText() + " / " + query.lastError().driverText();
    bool qfound = false;
    QString qname;
    int qtag;
    if(qresult == " / ")
    {
        while(query.next()) {
            qfound = true;
            qtag = query.record().field(0).value().toInt();
            qname = query.record().field(1).value().toString();
            break;
        }
    }
    if(qfound) {
        QSqlQuery del;
        QString sql = "delete from books_tags_link where tag = " + QString::number(qtag) + " and book = " + QString::number(bookIDSelected) +  ";";
        del.exec(sql);
        QString result = del.lastError().databaseText() + " / " + del.lastError().driverText();
        if(qresult == " / ")
        {
            qsql = "SELECT count(1) as count FROM tags t, books_tags_link btl where t.id=btl.tag and btl.tag = " + QString::number(qtag) + ";";
            query.exec(qsql);
            qresult = query.lastError().databaseText() + " / " + query.lastError().driverText();
            // Delete from tags if empty
            if(qresult == " / ")
            {
                qfound = false;
                int qcount = -1;
                while(query.next()) {
                    qfound = true;
                    qcount = query.record().field(0).value().toInt();
                    break;
                }
                if(qcount==0) {
                    QString sql = "delete from tags where id = " + QString::number(qtag) + ";";
                    del.exec(sql);
                    QString result = del.lastError().databaseText() + " / " + del.lastError().driverText();
                    if(qresult == " / ") {
                        qDebug() << "Tag: " << qname << " deleted";
                    } else {
                        qDebug() << "Database error: " + qresult;
                    }
                }
            } else {
                qDebug() << "Database error: " + qresult;
            }

        }
    }
//        qDebug() << "musicLibrary::addNewList Inserting new name";

    m_db->close();
    changeList(ActiveListIndex);
    return (qresult == " / " ? "" : qresult);
}
QString musicLibrary::addNewList(QString name)
{
    // This aborts if we can't open the database but returns other errors back to the caller (or blank for none)
    qDebug() << "Request to insert new playList " << name;
    m_db->open();
    checkSqlError("Opening SQL database " + m_db->databaseName(), m_db->lastError());
    QSqlQuery query;
    QSqlQuery insert;
    QString qsql = "select t.name from tags t where t.name like '" + name + "%' order by t.name desc;";
    query.exec(qsql);
    QString qresult = query.lastError().databaseText() + " / " + query.lastError().driverText();
    bool qfound = false;
    QString qname;
    if(qresult == " / ")
    {
//        qDebug() << "musicLibrary::addNewList Inserting new name";
        while(query.next()) {
            qfound = true;
            qname = query.record().field(0).value().toString();
            break;
        }

    }
    QString result;
    if(qfound) {
        qDebug() << "musicLibrary::addNewList Query name: " + qname;
    } else {
        qDebug() << "musicLibrary::addNewList No Query name found: ";
        QString sql = "insert into tags (name) values('" + name + "|01')";
        insert.exec(sql);
        result = insert.lastError().databaseText() + " / " + insert.lastError().driverText();
        if(result == " / ")  // If no error then also add the current book to it
        {
            qint32 rowid = insert.lastInsertId().toInt();
            sql = "insert into books_tags_link(tag, book) values (" + QString::number(rowid) + "," + QString::number(bookIDSelected) +  ");";
            insert.exec(sql);
            result = insert.lastError().databaseText() + " / " + insert.lastError().driverText();
            loadPlayLists();  // Need to reload since we added one -- this does not change active list though
        }
    }
    m_db->close();
    return (result == " / " ? "" : result);
}
