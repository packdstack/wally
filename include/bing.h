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

#ifndef BING_H
#define BING_H

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

#include "idcache.h"
#include "httpengine.h"

#define BING_ENGINE_NAME "Bing"
#define BING_API_KEY "define_here_your_api_key"

#define TAGS "tags"
#define TAGS_CONDITION "tagsCondition"
#define ADULT_FILTER "adultFilter"
#define LAST_PHOTO_URLS "lastPhotoUrls"

namespace Bing
{
  enum TagsCondition { And = 0, Or };
  enum AdultFilter { Off = 0, Moderate, Strict };

  class Item : public HttpEngine::Item
  {
    Q_OBJECT

    QStringList _tags;
    TagsCondition _tagsCondition;
    AdultFilter _adultFilter;

    QString photoTitle;
    QString photoDescription;
    IdCache<QUrl> _lastPhotoUrls;
    QUrl _photoUrl;
    QUrl _sourceUrl;
    QString photoOwner;
    QSize photoSize;

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
    bool isValidItem() const { return _tags.size(); }

  public:
    Item();
    Item(Item *item);
    virtual ~Item() { }

    void setTags(const QStringList &tags) { _tags = tags; }
    void setTagsCondition(TagsCondition tagsCondition) { _tagsCondition = tagsCondition; }
    void setAdultFilter(AdultFilter filter) { _adultFilter = filter; }

    AdultFilter adultFilter() const { return _adultFilter; }
    QStringList tags() const { return _tags; }
    TagsCondition tagsCondition() const { return _tagsCondition; }

    QString photoUrl() const { return _photoUrl.toString(); }

    bool equalTo(Engine::Item *item) const;
  };

  class DialogWidget : public XtEngine::DialogWidget
  {
    Q_OBJECT

    QLineEdit *leSearchFor;
    QComboBox *cbTagsCondition;
    QComboBox *cbAdultFilter;

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

    QPixmap buttonPixmap() const { return QPixmap(":/images/bing"); }
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

    QString name() const { return BING_ENGINE_NAME; }

    Item *newItem(const QVariant & /* data */ = QVariant()) { return new Item; }
    Item *newItem(Engine::Item *item) { return new Item(qobject_cast<Item *> (item)); }

    Engine::SettingsWidget *newSettingsWidget(QWidget *parent = 0) { return new SettingsWidget(this,parent); }

  public:
    Core(const QString &tempStorageDir, QObject *parent = 0) : HttpEngine::Core(tempStorageDir,parent) { }
    virtual ~Core() { }
  };
}

#endif
