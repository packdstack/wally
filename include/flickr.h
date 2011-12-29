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

#ifndef FLICKR_H
#define FLICKR_H

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

#include "idcache.h"
#include "httpengine.h"

#define FLICKR_ENGINE_NAME "Flickr"
#define FLICKR_API_KEY "99c82d50dc44a9d5bf7e54291f242511"
#define FLICKR_PAGES_HARD_LIMIT 4000

#define LAST_PHOTO_IDS "lastPhotoIds"
#define SEARCH_TAGS "tags"
#define SEARCH_TEXT "text"
#define TAGS_CONDITION "tagsCondition"
#define SEARCH_ORDER "searchOrder"
#define REQUESTED_SIZE "requestedSize"

namespace Flickr
{
  enum RequestedSize { Largest = 0, Smallest, Original, Square,
                       Thumbnail, Small, Medium, Large };
  enum TagsCondition { And = 0, Or };
  enum SearchOrder { DatePostedAsc = 0, DatePostedDesc, DateTakenAsc, DateTakenDesc,
                     InterestingnessAsc, InterestingnessDesc, Relevance };

  const QString searchOrderStrings[] = { "date-posted-asc", "date-posted-desc",
                                         "date-taken-asc", "date-taken-desc",
                                         "interestingness-asc", "interestingness-desc",
                                         "relevance" };

  class Item : public HttpEngine::Item
  {
    Q_OBJECT

    IdCache<QString> _lastPhotoIds;
    QString _photoId;
    QUrl _photoUrl;
    QString _text;
    QStringList _tags;
    TagsCondition _tagsCondition;
    SearchOrder _searchOrder;
    RequestedSize _requestedSize;

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
    void setTagsCondition(TagsCondition tagsCondition) { _tagsCondition = tagsCondition; }
    void setText(const QString &text) { _text = text; _tags.clear(); }
    void setRequestedSize(RequestedSize requestedSize) { _requestedSize = requestedSize; }
    void setSearchOrder(SearchOrder searchOrder) { _searchOrder = searchOrder; }

    RequestedSize requestedSize() const { return _requestedSize; }
    TagsCondition tagsCondition() const { return _tagsCondition; }
    SearchOrder searchOrder() const { return _searchOrder; }
    QString text() const { return _text; }
    QStringList tags() const { return _tags; }
    bool isText() const { return !_text.isEmpty(); }
    bool areTags() const { return _tags.size(); }

    QString photoUrl() const { return _photoUrl.toString(); }

    bool equalTo(Engine::Item *item) const;
  };

  class DialogWidget : public XtEngine::DialogWidget
  {
    Q_OBJECT

    QLineEdit *leSearchFor;
    QComboBox *cbTagsCondition;
    QRadioButton *rbTags;
    QRadioButton *rbText;
    QComboBox *cbRequestedSize;
    QComboBox *cbSearchOrder;

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

    QPixmap buttonPixmap() const { return QPixmap(":/images/flickr"); }
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

    QString name() const { return FLICKR_ENGINE_NAME; }

    Engine::SettingsWidget *newSettingsWidget(QWidget *parent = 0) { return new SettingsWidget(this,parent); }

  public:
    Core(const QString &tempStorageDir, QObject *parent = 0) : HttpEngine::Core(tempStorageDir,parent) { }
    virtual ~Core() { }
  };
}

#endif
