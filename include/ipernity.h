/*
 * Wally - Qt4 wallpaper/background changer
 * Copyright (C) 2009  Antonio Di Monaco <tony@becrux.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef IPERNITY_H
#define IPERNITY_H

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

#include "idcache.h"
#include "httpengine.h"

#define IPERNITY_ENGINE_NAME "Ipernity"
#define IPERNITY_API_KEY "e8315d400000273A461d49b1b40377fd"

#define SEARCH_TAGS "tags"
#define SEARCH_TEXT "text"
#define CONDITION "condition"
#define LAST_PHOTO_IDS "lastPhotoIds"

namespace Ipernity
{
  enum Condition { And = 0, Or };

  class Item : public HttpEngine::Item
  {
    Q_OBJECT

    QString _photoId;
    IdCache<QString> _lastPhotoIds;
    QUrl _photoUrl;
    QUrl _sourceUrl;
    QSize photoSize;
    QString photoOwner;
    QString photoDescription;
    QString photoTitle;
    QString photoLocation;
    QString _text;
    QStringList _tags;
    Condition _condition;

    int doCalculateNextIndex(bool randomMode, int currentIndex, int pages) const;
    QUrl doPrepareInit(HttpEngine::Operation &op);
    QUrl doPrepareSearch(HttpEngine::Operation &op);
    bool doProcessSearchResult(const QByteArray &response, int &newPagesCount);
    QUrl prepareSizeRequest(HttpEngine::Operation &op);
    QSize doProcessSizeRequestResult(const QByteArray &response);
    QUrl prepareDownload(HttpEngine::Operation &op);
    QUrl prepareInfoCollect(HttpEngine::Operation &op);
    PhotoInfo doProcessInfoCollectResult(const QByteArray &response);

    void doLoadSettings(QSettings &settings);
    void doSaveSettings(QSettings &settings) const;
    void doSaveState(QSettings &settings) const;

    QVariant data() const;
    bool isValidItem() const { return _tags.size() || _text.size(); }

  public:
    Item();
    Item(Item *item);
    virtual ~Item() { }

    void setTags(const QStringList &tags) { _tags = tags; _text.clear(); }
    void setCondition(Condition condition) { _condition = condition; }
    void setText(const QString &text) { _text = text; _tags.clear(); }

    QString text() const { return _text; }
    QStringList tags() const { return _tags; }
    Condition condition() const { return _condition; }
    bool isText() const { return !_text.isEmpty(); }
    bool areTags() const { return _tags.size(); }

    QString photoUrl() const { return _photoUrl.toString(); }

    bool equalTo(Engine::Item *item) const;
  };

  class DialogWidget : public XtEngine::DialogWidget
  {
    Q_OBJECT

    QLineEdit *leSearchFor;
    QComboBox *cbCondition;
    QRadioButton *rbTags;
    QRadioButton *rbText;

    XtEngine::Item *returnItem();
    void setupFromItem(XtEngine::Item *item = 0);

  public:
    DialogWidget(XtEngine::Core *core, QWidget *parent = 0);
    virtual ~DialogWidget() { }

    QSize sizeHint() const { return QSize(500,height()); }
  };

  class Dialog : public XtEngine::Dialog
  {
    Q_OBJECT

    XtEngine::DialogWidget *newDialogWidget(XtEngine::Core *core) { return new DialogWidget(core,this); }

  public:
    Dialog(XtEngine::Core *core, QWidget *parent = 0) : XtEngine::Dialog(core,parent) { }
    virtual ~Dialog() { }
  };

  class SettingsWidget : public HttpEngine::SettingsWidget
  {
    Q_OBJECT

    QPixmap buttonPixmap() const { return QPixmap(":/images/ipernity"); }
    QIcon buttonIcon() const { return QIcon(); }
    QString buttonText() const { return QString(); }

    XtEngine::Dialog *newDialog() { return new Dialog(qobject_cast<XtEngine::Core *> (core()),this); }

  public:
    SettingsWidget(Engine::Core *core, QWidget *parent = 0) : HttpEngine::SettingsWidget(core,parent) { }
    virtual ~SettingsWidget() { }
  };

  class Core : public HttpEngine::Core
  {
    Q_OBJECT

    Item *newItem(const QVariant & /* data */ = QVariant()) { return new Item; }
    Item *newItem(Engine::Item *item) { return new Item(qobject_cast<Item *> (item)); }

    QString name() const { return IPERNITY_ENGINE_NAME; }

    Engine::SettingsWidget *newSettingsWidget(QWidget *parent = 0) { return new SettingsWidget(this,parent); }

  public:
    Core(const QString &tempStorageDir, QObject *parent = 0) : HttpEngine::Core(tempStorageDir,parent) { }
    virtual ~Core() { }
  };
}

#endif
