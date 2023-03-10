#ifndef SETTINGSITEM_H
#define SETTINGSITEM_H

// Copyright 2023 by Linwood Ferguson, licensed under GNU GPLv3

#include <QWidget>

class MainWindow;
class settingsWidget;
class QLabel;
class QHBoxLayout;

class settingsItem : public QWidget
{
    Q_OBJECT
public:

    enum supportedTypes {int_t, uint_t, string_t, bool_t} ;
    supportedTypes thisType; // What type is this instance

    /* int */    explicit settingsItem(settingsWidget *sw, QWidget *parent, QString key, QString prompt, int lower, int higher);
    /* uint */   explicit settingsItem(settingsWidget *sw, QWidget *parent, QString key, QString prompt, unsigned int lower, unsigned int higher);
    /* string */ explicit settingsItem(settingsWidget *sw, QWidget *parent, QString key, QString prompt, QString property);
    /* bool */   explicit settingsItem(settingsWidget *sw, QWidget *parent, QString key, QString prompt);

    ~settingsItem();

    QLabel* msgPtr();  // Pointer to the message (error) label

    // Asserts if these are called other than for indicated type

    int toInt();                  /* int, uint */
    unsigned int toUInt();        /* uint */
    QString toString();           /* all, bool = "true" or "false" */
    bool toBool();                /* bool */

    int getPromptWidth();          // Get required space for label text
    void setPromptWidth(int);      // Force label to this size in px
    QLabel* m;
    QLabel* p;
    QHBoxLayout* hb;

    QWidget* valueWidget;   // Will contain one of data entry types

private:
    /* shared */ void settingsItemShared(settingsWidget *sw, QString key, QString prompt);
    settingsWidget* ourParent;
    MainWindow* mParent;
    int ourInt;
    unsigned int ourUInt;
    QString ourString;
    bool ourBool;
    bool validateRange(int bottom, int top);
    void paintEvent(QPaintEvent *);

signals:

public slots:
};

#endif // SETTINGSITEM_H
