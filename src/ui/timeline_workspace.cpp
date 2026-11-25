// Copyright 2026 Society of Indian Screen Editors Foundation
//
// AETHER
// Architect: Subhajit Singha, SISE
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "aether/ui/timeline_workspace.hpp"

#include "aether/ui/frame_label.hpp"

#include "aether/ui/placement_dialog.hpp"

#include "aether/core/media/import.hpp"

#include <limits>

#include <QAbstractItemView>
#include <QComboBox>
#include <QDataStream>
#include <QDialog>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFrame>
#include <QMap>
#include <QMenu>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMimeData>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QSignalBlocker>
#include <QPainter>
#include <QResizeEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QSizePolicy>
#include <QSlider>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

#include <optional>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <limits>

namespace aether {

constexpr int kRowHeight = 48;
constexpr int kRulerHeight = 28;

// Even ordinals are drawn as video and odd ordinals as audio. That choice is a
// view rule. It is not a stored kind. Numbers follow stored order within each
// letter: the earliest video is V1, the earliest audio is A1. The stack puts
// every video above the join and every audio below it. V1 and A1 sit on the join.
inline bool trackOrdinalIsVideo(int ordinal) { return ordinal % 2 == 0; }

template <typename Lane>
QString laneLabel(const Lane& lane, const std::vector<Lane>& lanes) {
  const bool video = trackOrdinalIsVideo(lane.ordinal);
  int number = 0;
  for (const Lane& other : lanes) {
    if (trackOrdinalIsVideo(other.ordinal) == video && other.ordinal <= lane.ordinal) {
      ++number;
    }
  }
  return QString(video ? "V%1" : "A%1").arg(number);
}

template <typename Lane>
void stackLanes(std::vector<Lane>& lanes) {
  std::stable_sort(lanes.begin(), lanes.end(), [](const Lane& left, const Lane& right) {
    const bool leftVideo = trackOrdinalIsVideo(left.ordinal);
    const bool rightVideo = trackOrdinalIsVideo(right.ordinal);
    if (leftVideo != rightVideo) {
      return leftVideo;
    }
    if (leftVideo) {
      return left.ordinal > right.ordinal;
    }
    return left.ordinal < right.ordinal;
  });
}

template <typename Lane>
int audioJoinIndex(const std::vector<Lane>& lanes) {
  for (int index = 0; index < static_cast<int>(lanes.size()); ++index) {
    if (!trackOrdinalIsVideo(lanes[static_cast<std::size_t>(index)].ordinal)) {
      return index;
    }
  }
  return -1;
}

class TimelineCanvas : public QWidget {
 public:
  struct Block {
    QString instanceId;
    QString title;
    std::int64_t recordStart = 0;
    std::int64_t recordEnd = 0;
    bool selected = false;
  };
  struct Row {
    QString trackId;
    int ordinal = 0;
    std::vector<Block> blocks;
  };

  explicit TimelineCanvas(QWidget* parent = nullptr) : QWidget(parent) {
    setObjectName("timelineCanvas");
    setAccessibleName("Timeline canvas");
    setAcceptDrops(true);
  }

  void setBlockClicked(std::function<void(const QString&)> handler) { blockClicked_ = std::move(handler); }

  void setDropRequested(std::function<void(const QString&, const QString&, std::int64_t, const QString&)> handler) {
    dropRequested_ = std::move(handler);
  }

  void setRulerClicked(std::function<void(const CanvasFrameHit&)> handler) { rulerClicked_ = std::move(handler); }
  void setExtractRangeMode(bool on) { rangeMode_ = on; }
  void setExtractClicked(std::function<void(const CanvasFrameHit&)> handler) { extractClicked_ = std::move(handler); }
  void setExtractRange(std::optional<std::int64_t> start, std::optional<std::int64_t> end) {
    rangeStart_ = start;
    rangeEnd_ = end;
    relayout();
  }

  void setPlayheadFrame(std::int64_t frame) {
    playheadFrame_ = frame < 0 ? 0 : frame;
    relayout();
  }

  std::int64_t playheadFrame() const { return playheadFrame_; }

  void setMarks(std::optional<std::int64_t> markIn, std::optional<std::int64_t> markOut) {
    markIn_ = markIn;
    markOut_ = markOut;
    relayout();
  }

  QPoint blockCenter(const QString& instanceId) const {
    const QRect box = blockRect(instanceId);
    return box.isEmpty() ? QPoint() : box.center();
  }

  bool blockIsSelected(const QString& instanceId) const {
    for (const Row& row : rows_) {
      for (const Block& block : row.blocks) {
        if (block.instanceId == instanceId) {
          return block.selected;
        }
      }
    }
    return false;
  }

  bool deliver(QEvent* event) { return QWidget::event(event); }

  void setRows(std::vector<Row> rows) {
    stackLanes(rows);
    rows_ = std::move(rows);
    relayout();
  }

  const std::vector<Row>& rows() const { return rows_; }

  int viewPixels() const { return viewPixels_; }

  std::int64_t visibleEnd() const { return visibleEnd_; }

  // View scale only. The default is the tested paint constant. Nothing is stored.
  void setViewPixels(int pixels) {
    if (pixels < 1) {
      pixels = 1;
    }
    if (pixels > 32) {
      pixels = 32;
    }
    if (pixels == viewPixels_) {
      return;
    }
    viewPixels_ = pixels;
    relayout();
  }

 protected:
  void dragEnterEvent(QDragEnterEvent* event) override { dragMoveEvent(event); }

  void dragMoveEvent(QDragMoveEvent* event) override {
    const QString reason = previewAt(event->position().toPoint());
    if (!reason.isEmpty() || !event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
      previewFrame_ = -1;
      event->ignore();
      update();
      return;
    }
    event->acceptProposedAction();
    update();
  }

  void dragLeaveEvent(QDragLeaveEvent*) override {
    previewFrame_ = -1;
    update();
  }

  void dropEvent(QDropEvent* event) override {
    const QPoint pos = event->position().toPoint();
    const QString reason = previewAt(pos);
    const QString objectId = objectIdFromMime(event->mimeData());
    previewFrame_ = -1;
    update();
    if (!dropRequested_) {
      event->ignore();
      return;
    }
    if (objectId.isEmpty()) {
      dropRequested_({}, {}, 0, "the drop has no registered reference");
      event->ignore();
      return;
    }
    if (!reason.isEmpty()) {
      dropRequested_(objectId, {}, 0, reason);
      event->ignore();
      return;
    }
    const int row = rowIndexAt(pos.y());
    const QString trackId = row >= 0 ? rows_[static_cast<std::size_t>(row)].trackId : QString();
    dropRequested_(objectId, trackId, previewHeld_, QString());
    event->acceptProposedAction();
  }

  void mousePressEvent(QMouseEvent* event) override {
    const QPoint pos = event->position().toPoint();
    if (rangeMode_ && event->button() == Qt::LeftButton) {
      if (extractClicked_) {
        extractClicked_(frameAtCanvasX(pos.x(), viewPixels_, kTimelineOriginPx));
      }
      return;
    }
    if (event->button() == Qt::LeftButton && pos.y() >= 0 && pos.y() < kRulerHeight) {
      if (rulerClicked_) {
        rulerClicked_(frameAtCanvasX(pos.x(), viewPixels_, kTimelineOriginPx));
      }
      return;
    }
    // The red playhead is the display playhead. Dragging it is not scrubbing.
    const int playhead = paintX(playheadFrame_);
    if (event->button() == Qt::LeftButton && playhead >= 0 && pos.y() >= kRulerHeight &&
        std::abs(pos.x() - playhead) <= 2) {
      return;
    }
    int top = kRulerHeight;
    for (const Row& row : rows_) {
      if (pos.y() >= top && pos.y() < top + kRowHeight) {
        for (const Block& block : row.blocks) {
          const QRect box = paintedRect(block, top);
          if (!box.isEmpty() && box.contains(pos)) {
            if (blockClicked_) {
              blockClicked_(block.instanceId);
            }
            return;
          }
        }
      }
      top += kRowHeight;
    }
  }

  void paintEvent(QPaintEvent*) override {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(32, 32, 32));
    painter.setPen(QColor(180, 180, 180));
    painter.drawText(8, 18, "frames");
    int top = kRulerHeight;
    if (rows_.empty()) {
      painter.drawText(8, top + 20, "No tracks");
    }
    for (const Row& row : rows_) {
      painter.fillRect(QRect(0, top, width(), kRowHeight), QColor(24, 24, 24));
      for (const Block& block : row.blocks) {
        const QRect box = paintedRect(block, top);
        if (box.isEmpty()) {
          continue;
        }
        painter.fillRect(box, block.selected ? QColor(78, 132, 156) : QColor(58, 96, 118));
        painter.setPen(QColor(180, 200, 214));
        painter.drawRect(box);
        const QString span = QString::number(block.recordStart) + "-" + QString::number(block.recordEnd);
        const QString label = block.title.isEmpty() ? span : block.title + "  " + span;
        painter.setPen(QColor(230, 234, 238));
        painter.save();
        painter.setClipRect(box.adjusted(2, 0, -2, 0));
        painter.drawText(box.adjusted(6, 0, -4, 0), Qt::AlignLeft | Qt::AlignVCenter, label);
        painter.restore();
      }
      painter.setPen(QColor(80, 80, 80));
      painter.drawLine(0, top + kRowHeight - 1, width(), top + kRowHeight - 1);
      top += kRowHeight;
    }
    const int join = audioJoinIndex(rows_);
    if (join > 0) {
      const int y = kRulerHeight + join * kRowHeight;
      painter.setPen(QPen(QColor(196, 202, 210), 2));
      painter.drawLine(0, y, width(), y);
    }
    if (rangeStart_.has_value() && rangeEnd_.has_value() && *rangeEnd_ > *rangeStart_) {
      const int x0 = paintX(*rangeStart_);
      const int x1 = paintX(*rangeEnd_);
      if (x0 >= 0 && x1 > x0) {
        painter.fillRect(QRect(x0, 0, x1 - x0, height()), QColor(90, 140, 190, 70));
      }
    }
    const int standing = paintX(playheadFrame_);
    if (standing >= 0) {
      painter.fillRect(QRect(standing, 0, 2, height()), QColor(214, 69, 69));
    }
    drawMark(painter, markIn_, QColor(90, 190, 130), "IN", height());
    drawMark(painter, markOut_, QColor(220, 170, 70), "OUT", height());
    if (previewFrame_ >= 0) {
      const int mark = paintX(previewFrame_);
      if (mark >= 0) {
        painter.setPen(QColor(214, 69, 69));
        painter.drawLine(mark, 0, mark, height());
      }
    }
  }

 private:
  int paintX(std::int64_t frame) const {
    const std::optional<int> x = timelinePaintX(frame, viewPixels_, kTimelineOriginPx);
    return x ? *x : -1;
  }

  void relayout() {
    std::int64_t end = 24;
    for (const Row& row : rows_) {
      for (const Block& block : row.blocks) {
        if (block.recordEnd > end) {
          end = block.recordEnd;
        }
      }
    }
    auto consider = [&end, this](const std::optional<std::int64_t>& frame) {
      if (!frame.has_value()) {
        return;
      }
      if (!timelinePaintX(*frame, viewPixels_, kTimelineOriginPx).has_value()) {
        return;
      }
      if (*frame + 1 > end) {
        end = *frame + 1;
      }
    };
    consider(markIn_);
    consider(markOut_);
    consider(rangeEnd_);
    consider(std::optional<std::int64_t>(playheadFrame_));
    visibleEnd_ = end;
    const int width = paintX(end) + 8;
    const int height = kRulerHeight + static_cast<int>(rows_.size()) * kRowHeight + 8;
    setMinimumSize(std::max(width, 240), std::max(height, kRulerHeight + 8));
    update();
  }

  void drawMark(QPainter& painter, const std::optional<std::int64_t>& frame, const QColor& color,
                       const char* label, int widgetHeight) {
    if (!frame.has_value() || widgetHeight <= 0) {
      return;
    }
    const int x = paintX(*frame);
    if (x < 0) {
      return;
    }
    painter.fillRect(QRect(x, 0, 2, widgetHeight), color);
    painter.setPen(color);
    painter.drawText(x + 4, 12, label);
  }

  static QString objectIdFromMime(const QMimeData* mime) {
    if (mime == nullptr || !mime->hasFormat("application/x-qabstractitemmodeldatalist")) {
      return {};
    }
    QByteArray encoded = mime->data("application/x-qabstractitemmodeldatalist");
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    while (!stream.atEnd()) {
      int row = 0;
      int column = 0;
      QMap<int, QVariant> roles;
      stream >> row >> column >> roles;
      const QString objectId = roles.value(Qt::UserRole).toString();
      if (!objectId.isEmpty()) {
        return objectId;
      }
    }
    return {};
  }

  int rowIndexAt(int y) const {
    if (y < kRulerHeight) {
      return -2;
    }
    if (rows_.empty()) {
      return -1;
    }
    const int index = (y - kRulerHeight) / kRowHeight;
    if (index < 0 || index >= static_cast<int>(rows_.size())) {
      return -2;
    }
    return index;
  }

  QString previewAt(const QPoint& pos) {
    const CanvasFrameHit hit = frameAtCanvasX(pos.x(), viewPixels_, kTimelineOriginPx);
    if (!hit.ok) {
      previewHeld_ = -1;
      return QString::fromUtf8(hit.reason);
    }
    const int row = rowIndexAt(pos.y());
    if (row == -2) {
      previewHeld_ = -1;
      return "the pointer is not on a track";
    }
    previewHeld_ = hit.frame;
    previewFrame_ = hit.frame;
    return {};
  }

  QRect paintedRect(const Block& block, int top) const {
    const int left = paintX(block.recordStart);
    const int right = paintX(block.recordEnd);
    if (left < 0 || right <= left) {
      return {};
    }
    return QRect(left, top + 4, right - left, kRowHeight - 8);
  }

  QRect blockRect(const QString& instanceId) const {
    int top = kRulerHeight;
    for (const Row& row : rows_) {
      for (const Block& block : row.blocks) {
        if (block.instanceId == instanceId) {
          return paintedRect(block, top);
        }
      }
      top += kRowHeight;
    }
    return {};
  }

  std::vector<Row> rows_;
  int viewPixels_ = kTimelinePixelsPerFrame;
  std::int64_t visibleEnd_ = 24;
  std::function<void(const QString&)> blockClicked_;
  std::function<void(const QString&, const QString&, std::int64_t, const QString&)> dropRequested_;
  std::function<void(const CanvasFrameHit&)> rulerClicked_;
  std::function<void(const CanvasFrameHit&)> extractClicked_;
  bool rangeMode_ = false;
  std::optional<std::int64_t> rangeStart_;
  std::optional<std::int64_t> rangeEnd_;
  std::optional<std::int64_t> markIn_;
  std::optional<std::int64_t> markOut_;
  std::int64_t previewFrame_ = -1;
  std::int64_t previewHeld_ = -1;
  std::int64_t playheadFrame_ = 0;
};

// Headers stay fixed while the sequence scrolls sideways. Enable, lock, and the
// V/A glyph are drawn and are not stored. A click on the lane name toggles that
// track as an Extract participant. The id is session UI state. The name is the
// presentation label, V1 or A1, not the stored ordinal.
class TrackHeaderColumn : public QWidget {
 public:
  struct Track {
    QString trackId;
    int ordinal = 0;
  };

  explicit TrackHeaderColumn(QWidget* parent = nullptr) : QWidget(parent) {
    setObjectName("timelineTrackHeaders");
    setAccessibleName("Track headers");
    setFixedWidth(210);
    setToolTip("Click a track name to include that track in Extract. Enable and lock do nothing. The V or A mark is a view label. A track kind is not stored.");
    publishAccessible();
  }

  void setTracks(const std::vector<Track>& tracks) {
    QSet<QString> live;
    for (const Track& track : tracks) {
      if (!track.trackId.isEmpty()) {
        live.insert(track.trackId);
      }
    }
    QSet<QString> kept;
    for (const QString& id : selected_) {
      if (live.contains(id)) {
        kept.insert(id);
      }
    }
    selected_ = kept;
    tracks_ = tracks;
    stackLanes(tracks_);
    publishAccessible();
    update();
  }

  void clearParticipants() {
    if (selected_.isEmpty()) {
      return;
    }
    selected_.clear();
    publishAccessible();
    update();
  }

  QStringList participantIds() const {
    std::vector<const Track*> chosen;
    for (const Track& track : tracks_) {
      if (selected_.contains(track.trackId)) {
        chosen.push_back(&track);
      }
    }
    std::sort(chosen.begin(), chosen.end(), [](const Track* left, const Track* right) {
      return left->ordinal < right->ordinal;
    });
    QStringList ids;
    for (const Track* track : chosen) {
      ids.append(track->trackId);
    }
    return ids;
  }

  void setVerticalOffset(int offset) {
    offset_ = offset;
    update();
  }

 protected:
  void mousePressEvent(QMouseEvent* event) override {
    if (event->button() == Qt::LeftButton) {
      const QPoint pos = event->position().toPoint();
      for (int index = 0; index < static_cast<int>(tracks_.size()); ++index) {
        if (!nameRect(index).contains(pos)) {
          continue;
        }
        const QString id = tracks_[static_cast<std::size_t>(index)].trackId;
        if (id.isEmpty()) {
          break;
        }
        if (selected_.contains(id)) {
          selected_.remove(id);
        } else {
          selected_.insert(id);
        }
        publishAccessible();
        update();
        break;
      }
    }
    event->accept();
  }

  void paintEvent(QPaintEvent*) override {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(22, 24, 28));
    painter.setPen(QColor(90, 96, 104));
    painter.drawLine(width() - 1, 0, width() - 1, height());
    if (tracks_.empty()) {
      painter.setPen(QColor(140, 146, 154));
      painter.drawText(8, kRulerHeight + 20, "No tracks");
      return;
    }
    for (std::size_t index = 0; index < tracks_.size(); ++index) {
      const bool participating = selected_.contains(tracks_[index].trackId);
      const bool video = trackOrdinalIsVideo(tracks_[index].ordinal);
      const QString label = laneLabel(tracks_[index], tracks_);
      const int top = kRulerHeight + static_cast<int>(index) * kRowHeight - offset_;
      if (top + kRowHeight < 0 || top > height()) {
        continue;
      }
      if (participating) {
        painter.fillRect(QRect(4, top, width() - 5, kRowHeight - 1), QColor(28, 58, 86));
      }
      const QColor rail = video ? QColor(214, 168, 64) : QColor(96, 156, 186);
      painter.fillRect(QRect(0, top, 4, kRowHeight), participating ? QColor(120, 186, 232) : rail);
      painter.setPen(rail);
      painter.drawText(QRect(8, top, 36, kRowHeight), Qt::AlignVCenter | Qt::AlignLeft, label);
      const QRect name = nameRect(static_cast<int>(index));
      if (participating) {
        painter.fillRect(name.adjusted(0, 8, 0, -8), QColor(46, 118, 176));
        painter.setPen(QColor(255, 255, 255));
      } else {
        painter.setPen(QColor(214, 218, 224));
      }
      painter.drawText(name, Qt::AlignVCenter | Qt::AlignLeft, label);
      drawEye(painter, QRect(150, top + 14, 20, 18));
      drawLock(painter, QRect(178, top + 13, 18, 20));
      painter.setPen(QColor(48, 52, 58));
      painter.drawLine(0, top + kRowHeight - 1, width(), top + kRowHeight - 1);
    }
    const int join = audioJoinIndex(tracks_);
    if (join > 0) {
      const int y = kRulerHeight + join * kRowHeight - offset_;
      painter.setPen(QPen(QColor(196, 202, 210), 2));
      painter.drawLine(0, y, width(), y);
    }
  }

 private:
  QRect nameRect(int index) const {
    const int top = kRulerHeight + index * kRowHeight - offset_;
    return QRect(46, top, 96, kRowHeight);
  }

  void publishAccessible() {
    if (selected_.isEmpty()) {
      setAccessibleDescription("No extract tracks");
      return;
    }
    std::vector<const Track*> chosen;
    for (const Track& track : tracks_) {
      if (selected_.contains(track.trackId)) {
        chosen.push_back(&track);
      }
    }
    std::sort(chosen.begin(), chosen.end(), [](const Track* left, const Track* right) {
      return left->ordinal < right->ordinal;
    });
    QStringList labels;
    for (const Track* track : chosen) {
      labels.append(laneLabel(*track, tracks_));
    }
    setAccessibleDescription(QString("Extract tracks %1").arg(labels.join(", ")));
  }

  static void drawEye(QPainter& painter, const QRect& box) {
    painter.setPen(QColor(186, 192, 200));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(box.adjusted(1, 3, -1, -3));
    painter.setBrush(QColor(186, 192, 200));
    painter.drawEllipse(QRect(box.center().x() - 2, box.center().y() - 2, 4, 4));
  }

  static void drawLock(QPainter& painter, const QRect& box) {
    painter.setPen(QColor(186, 192, 200));
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(QRect(box.left() + 3, box.top(), 10, 10), 0, 180 * 16);
    painter.fillRect(QRect(box.left() + 2, box.top() + 7, 12, 9), QColor(186, 192, 200));
  }

  std::vector<Track> tracks_;
  QSet<QString> selected_;
  int offset_ = 0;
};

// The strip and the zoom slider change only the scroll and the view scale.
class TimelineNavigator : public QWidget {
 public:
  TimelineNavigator(TimelineCanvas* canvas, QScrollArea* scroll, QWidget* parent = nullptr)
      : QWidget(parent), canvas_(canvas), scroll_(scroll) {
    setObjectName("timelineNavigator");
    setAccessibleName("Timeline overview");
    setMinimumHeight(48);
    setMouseTracking(false);
    zoom_ = new QSlider(Qt::Horizontal, this);
    zoom_->setObjectName("timelineZoom");
    zoom_->setAccessibleName("Timeline zoom");
    zoom_->setRange(1, 16);
    zoom_->setValue(kTimelinePixelsPerFrame);
    zoom_->setToolTip("Zoom changes only the on-screen timeline scale.");
    zoom_->setStyleSheet(
        "QSlider::groove:horizontal { height: 4px; background: #3a414c; border-radius: 2px; }"
        "QSlider::handle:horizontal { width: 12px; height: 12px; margin: -5px 0; background: #d5dbe3; "
        "border-radius: 6px; }");
    connect(zoom_, &QSlider::valueChanged, this, [this](int value) {
      if (canvas_ == nullptr || scroll_ == nullptr) {
        return;
      }
      QScrollBar* bar = scroll_->horizontalScrollBar();
      const int oldPixels = canvas_->viewPixels() > 0 ? canvas_->viewPixels() : 1;
      const int leftFrame = bar->value() / oldPixels;
      canvas_->setViewPixels(value);
      bar->setValue(leftFrame * canvas_->viewPixels());
      update();
    });
    connect(scroll_->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this](int) { update(); });
    connect(scroll_->horizontalScrollBar(), &QScrollBar::rangeChanged, this, [this](int, int) { update(); });
  }

 protected:
  void resizeEvent(QResizeEvent*) override {
    zoom_->setGeometry(width() - 132, (height() - 18) / 2, 120, 18);
  }

  void mousePressEvent(QMouseEvent* event) override {
    if (event->button() != Qt::LeftButton || !stripRect().contains(event->position().toPoint())) {
      return;
    }
    dragging_ = true;
    pressX_ = event->position().toPoint().x();
    pressScroll_ = scroll_->horizontalScrollBar()->value();
    grabMouse();
    event->accept();
  }

  void mouseMoveEvent(QMouseEvent* event) override {
    if (!dragging_ || canvas_ == nullptr) {
      return;
    }
    const QRect strip = stripRect();
    const std::int64_t end = canvas_->visibleEnd() > 0 ? canvas_->visibleEnd() : 1;
    const int width = strip.width() > 0 ? strip.width() : 1;
    const std::int64_t deltaFrames = (static_cast<std::int64_t>(event->position().toPoint().x() - pressX_) * end) / width;
    const int next = pressScroll_ + static_cast<int>(deltaFrames * canvas_->viewPixels());
    scroll_->horizontalScrollBar()->setValue(next);
    event->accept();
  }

  void mouseReleaseEvent(QMouseEvent* event) override {
    if (dragging_) {
      releaseMouse();
    }
    dragging_ = false;
    event->accept();
  }

  void paintEvent(QPaintEvent*) override {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(16, 17, 20));
    const QRect strip = stripRect();
    painter.fillRect(strip, QColor(28, 30, 34));
    if (canvas_ == nullptr) {
      return;
    }
    const std::int64_t end = canvas_->visibleEnd() > 0 ? canvas_->visibleEnd() : 1;
    const auto& rows = canvas_->rows();
    const int bands = rows.empty() ? 1 : static_cast<int>(rows.size());
    for (int index = 0; index < static_cast<int>(rows.size()); ++index) {
      const int top = strip.top() + index * strip.height() / bands;
      const int band = std::max(2, strip.height() / bands - 2);
      for (const TimelineCanvas::Block& block : rows[static_cast<std::size_t>(index)].blocks) {
        if (block.recordEnd <= block.recordStart) {
          continue;
        }
        const int left = strip.left() + static_cast<int>((block.recordStart * strip.width()) / end);
        int right = strip.left() + static_cast<int>((block.recordEnd * strip.width()) / end);
        if (right <= left) {
          right = left + 2;
        }
        painter.fillRect(QRect(left, top + 1, right - left, band), QColor(58, 96, 118));
      }
    }
    const QRect range = visibleRange(strip, end);
    painter.setPen(QPen(QColor(232, 236, 242), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(range.adjusted(1, 1, -2, -2));
    painter.fillRect(QRect(range.left(), range.top(), 3, range.height()), QColor(232, 236, 242));
    painter.fillRect(QRect(range.right() - 3, range.top(), 3, range.height()), QColor(232, 236, 242));
  }

 private:
  QRect stripRect() const { return QRect(8, 8, std::max(8, width() - 148), std::max(8, height() - 16)); }

  QRect visibleRange(const QRect& strip, std::int64_t end) const {
    const int pixels = canvas_->viewPixels() > 0 ? canvas_->viewPixels() : 1;
    const std::int64_t start = scroll_->horizontalScrollBar()->value() / pixels;
    std::int64_t count = scroll_->viewport()->width() / pixels;
    if (count < 1) {
      count = 1;
    }
    int left = strip.left() + static_cast<int>((start * strip.width()) / end);
    int span = static_cast<int>((count * strip.width()) / end);
    if (span < 8) {
      span = 8;
    }
    if (left < strip.left()) {
      left = strip.left();
    }
    if (left + span > strip.right()) {
      left = std::max(strip.left(), strip.right() - span);
    }
    return QRect(left, strip.top() + 2, span, std::max(4, strip.height() - 4));
  }

  TimelineCanvas* canvas_ = nullptr;
  QScrollArea* scroll_ = nullptr;
  QSlider* zoom_ = nullptr;
  bool dragging_ = false;
  int pressX_ = 0;
  int pressScroll_ = 0;
};

namespace {

// QToolButton::InstantPopup calls QMenu::exec from the mouse press. QMenu then
// hides when that press is still outside the menu and the pointer has not moved.
// A menu-bar menu survives the same press because QMenu forwards it to the menu
// bar. A tool button is not given that exception, so the opening press dismisses
// the menu before a window stays mapped. This button keeps the real QMenu and
// opens it from the finished click, after that press is done.
class PanelMenuButton : public QToolButton {
 public:
  explicit PanelMenuButton(QWidget* parent = nullptr) : QToolButton(parent) {}

 protected:
  void mousePressEvent(QMouseEvent* event) override {
    if (event->button() != Qt::LeftButton) {
      QToolButton::mousePressEvent(event);
      return;
    }
    pressInside_ = hitButton(event->position().toPoint());
    if (!pressInside_) {
      event->ignore();
      return;
    }
    setDown(true);
    event->accept();
  }

  void mouseReleaseEvent(QMouseEvent* event) override {
    if (event->button() != Qt::LeftButton || !pressInside_) {
      QToolButton::mouseReleaseEvent(event);
      return;
    }
    pressInside_ = false;
    setDown(false);
    if (hitButton(event->position().toPoint())) {
      emit clicked();
    }
    event->accept();
  }

 private:
  bool pressInside_ = false;
};

QToolButton* panelMenuButton(const QString& objectName, const QString& accessibleName, QWidget* parent) {
  auto* button = new PanelMenuButton(parent);
  button->setObjectName(objectName);
  button->setText("Menu");
  button->setAccessibleName(accessibleName);
  button->setAutoRaise(true);
  button->setPopupMode(QToolButton::DelayedPopup);
  auto* menu = new QMenu(button);
  menu->setObjectName(objectName + "Popup");
  button->setMenu(menu);
  QObject::connect(button, &QToolButton::clicked, button, [button, menu]() {
    if (menu->isVisible()) {
      menu->hide();
      return;
    }
    menu->popup(button->mapToGlobal(QPoint(0, button->height())));
  });
  return button;
}

QWidget* menuForm(QMenu* menu) {
  auto* form = new QWidget(menu);
  auto* layout = new QVBoxLayout(form);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(4);
  form->setMinimumWidth(360);
  return form;
}

void addMenuForm(QMenu* menu, QWidget* form) {
  auto* action = new QWidgetAction(menu);
  action->setDefaultWidget(form);
  menu->addAction(action);
}

}  // namespace

TimelineWorkspace::TimelineWorkspace(ProjectSession& session, QWidget* parent)
    : QWidget(parent), session_(session) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  auto* vertical = new QSplitter(Qt::Vertical, this);
  vertical->setObjectName("editRoomSplit");
  layout->addWidget(vertical);

  auto* upper = new QSplitter(Qt::Horizontal, vertical);
  upper->setObjectName("editUpperSplit");

  auto* project = new QFrame(upper);
  project->setObjectName("projectPanel");
  auto* projectLayout = new QVBoxLayout(project);
  projectLayout->setContentsMargins(8, 6, 8, 6);
  projectLayout->setSpacing(4);
  auto* projectHeader = new QHBoxLayout();
  auto* projectTitle = new QLabel("Project Contents", project);
  projectTitle->setObjectName("panelTitle");
  projectHeader->addWidget(projectTitle, 1);
  auto* projectMenuButton = panelMenuButton("projectPanelMenu", "Project Contents menu", project);
  projectHeader->addWidget(projectMenuButton);
  projectLayout->addLayout(projectHeader);
  auto* projectBody = new QWidget(project);
  projectBody->setObjectName("projectBody");
  auto* projectBodyLayout = new QVBoxLayout(projectBody);
  projectBodyLayout->setContentsMargins(0, 0, 0, 0);
  projectBodyLayout->setSpacing(4);
  auto* sequenceHeading = new QLabel("Sequences", projectBody);
  sequenceHeading->setObjectName("sequenceHeading");
  projectBodyLayout->addWidget(sequenceHeading);
  sequences_ = new QListWidget(projectBody);
  sequences_->setObjectName("sequenceList");
  sequences_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  sequences_->setSelectionMode(QAbstractItemView::SingleSelection);
  sequences_->setMinimumHeight(120);
  projectBodyLayout->addWidget(sequences_, 1);
  projectLayout->addWidget(projectBody, 1);

  auto* viewer = new QFrame(upper);
  viewer->setObjectName("viewerPanel");
  auto* viewerLayout = new QVBoxLayout(viewer);
  viewerLayout->setContentsMargins(8, 6, 8, 6);
  viewerLayout->setSpacing(4);
  auto* viewerHeader = new QHBoxLayout();
  viewerHeader->setSpacing(6);
  sourceTab_ = new QPushButton("Source", viewer);
  sourceTab_->setObjectName("viewerSourceTab");
  sourceTab_->setCheckable(true);
  sourceTab_->setProperty("viewerTab", true);
  sourceTab_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  sequenceTab_ = new QPushButton("No sequence", viewer);
  sequenceTab_->setObjectName("viewerRecordTab");
  sequenceTab_->setCheckable(true);
  sequenceTab_->setChecked(true);
  sequenceTab_->setProperty("viewerTab", true);
  sequenceTab_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  viewerHeader->addWidget(sourceTab_);
  viewerHeader->addWidget(sequenceTab_);
  viewerHeader->addStretch(1);
  auto* viewerMenuButton = panelMenuButton("viewerPanelMenu", "Sequence viewer menu", viewer);
  viewerHeader->addWidget(viewerMenuButton);
  viewerLayout->addLayout(viewerHeader);
  auto* viewerBody = new QWidget(viewer);
  viewerBody->setObjectName("viewerBody");
  auto* viewerBodyLayout = new QVBoxLayout(viewerBody);
  viewerBodyLayout->setContentsMargins(0, 0, 0, 0);
  viewerBodyLayout->setSpacing(4);
  viewerLayout->addWidget(viewerBody, 1);

  auto* side = new QFrame(upper);
  side->setObjectName("sidePanel");
  auto* sideLayout = new QVBoxLayout(side);
  sideLayout->setContentsMargins(0, 0, 0, 0);
  sideLayout->setSpacing(0);
  auto* sideBody = new QWidget(side);
  sideBody->setObjectName("sideBody");
  auto* sideBodyLayout = new QVBoxLayout(sideBody);
  sideBodyLayout->setContentsMargins(0, 0, 0, 0);
  sideBodyLayout->setSpacing(0);
  auto* sideHeader = new QHBoxLayout();
  sideHeader->setContentsMargins(8, 4, 8, 0);
  sideHeader->addStretch(1);
  auto* sideMenuButton = panelMenuButton("sidePanelMenu", "Side panel menu", side);
  sideMenuButton->menu()->addAction("No panel commands")->setEnabled(false);
  sideHeader->addWidget(sideMenuButton);
  sideLayout->addLayout(sideHeader);
  sideLayout->addWidget(sideBody, 1);

  upper->addWidget(project);
  upper->addWidget(viewer);
  upper->addWidget(side);
  upper->setStretchFactor(0, 3);
  upper->setStretchFactor(1, 5);
  upper->setStretchFactor(2, 3);
  upper->setSizes({420, 760, 420});

  auto* timeline = new QFrame(vertical);
  timeline->setObjectName("timelinePanel");
  auto* timelineLayout = new QVBoxLayout(timeline);
  timelineLayout->setContentsMargins(8, 4, 8, 6);
  timelineLayout->setSpacing(4);
  auto* toolbar = new QHBoxLayout();
  toolbar->setSpacing(4);
  auto* timelineTitle = new QLabel("Timeline", timeline);
  timelineTitle->setObjectName("panelTitle");
  toolbar->addWidget(timelineTitle);
  auto* recordInButton = new QPushButton("Mark In", timeline);
  recordInButton->setObjectName("recordMarkIn");
  auto* recordOutButton = new QPushButton("Mark Out", timeline);
  recordOutButton->setObjectName("recordMarkOut");
  auto* overwriteButton = new QPushButton("Overwrite", timeline);
  overwriteButton->setObjectName("overwriteApply");
  auto* splitButton = new QPushButton("Split", timeline);
  splitButton->setObjectName("splitApply");
  auto* liftButton = new QPushButton("Lift", timeline);
  liftButton->setObjectName("liftApply");
  toolbar->addWidget(recordInButton);
  toolbar->addWidget(recordOutButton);
  toolbar->addWidget(overwriteButton);
  toolbar->addWidget(splitButton);
  toolbar->addWidget(liftButton);
  extractButton_ = new QPushButton("Extract", timeline);
  extractButton_->setObjectName("extractApply");
  toolbar->addWidget(extractButton_);
  auto* timelinePlay = new QPushButton("Play", timeline);
  timelinePlay->setObjectName("timelinePlay");
  auto* timelineStop = new QPushButton("Stop", timeline);
  timelineStop->setObjectName("timelineStop");
  toolbar->addWidget(timelinePlay);
  toolbar->addWidget(timelineStop);
  toolbar->addStretch(1);
  connect(timelinePlay, &QPushButton::clicked, this, [this]() { emit timelinePlayRequested(); });
  connect(timelineStop, &QPushButton::clicked, this, [this]() { emit timelineStopRequested(); });
  connect(extractButton_, &QPushButton::clicked, this, &TimelineWorkspace::applyExtract);
  connect(recordInButton, &QPushButton::clicked, this, &TimelineWorkspace::markRecordIn);
  connect(recordOutButton, &QPushButton::clicked, this, &TimelineWorkspace::markRecordOut);
  auto* timelineMenuButton = panelMenuButton("timelinePanelMenu", "Timeline menu", timeline);
  toolbar->addWidget(timelineMenuButton);
  timelineLayout->addLayout(toolbar);

  timelineClock_ = new QLabel("—", timeline);
  timelineClock_->setObjectName("timelineClock");
  timelineLayout->addWidget(timelineClock_);
  rulerCaption_ = new QLabel("No sequence selected. Drop-frame flag absent.", timeline);
  rulerCaption_->setObjectName("rulerCaption");
  timelineLayout->addWidget(rulerCaption_);

  auto* extractRow = new QHBoxLayout();
  extractRow->setSpacing(6);
  extractRangeMode_ = new QPushButton("Range", timeline);
  extractRangeMode_->setObjectName("extractRangeMode");
  extractRangeMode_->setCheckable(true);
  extractStart_ = new QLineEdit(timeline);
  extractStart_->setObjectName("extractRangeStart");
  extractStart_->setPlaceholderText("start");
  extractStart_->setMaximumWidth(96);
  extractEnd_ = new QLineEdit(timeline);
  extractEnd_->setObjectName("extractRangeEnd");
  extractEnd_->setPlaceholderText("end");
  extractEnd_->setMaximumWidth(96);
  visibleEnd_ = new QLabel("Visible end none", timeline);
  visibleEnd_->setObjectName("visibleEnd");
  extractRangeReadout_ = new QLabel("No record range.", timeline);
  extractRangeReadout_->setObjectName("extractRangeReadout");
  extractRow->addWidget(extractRangeMode_);
  extractRow->addWidget(extractStart_);
  extractRow->addWidget(extractEnd_);
  extractRow->addWidget(visibleEnd_);
  extractRow->addWidget(extractRangeReadout_, 1);
  timelineLayout->addLayout(extractRow);
  connect(extractStart_, &QLineEdit::editingFinished, this, [this]() { publishExtractRange(); });
  connect(extractEnd_, &QLineEdit::editingFinished, this, [this]() { publishExtractRange(); });

  canvas_ = new TimelineCanvas(timeline);
  connect(extractRangeMode_, &QPushButton::toggled, this, [this](bool on) { canvas_->setExtractRangeMode(on); });
  canvas_->setBlockClicked([this](const QString& instanceId) { selectInstance(instanceId); });
  canvas_->setRulerClicked([this](const CanvasFrameHit& hit) {
    if (!hit.ok) {
      recordMarkReason_->setText(QString::fromUtf8(hit.reason));
      return;
    }
    recordMarkFrame_->setText(QString::number(hit.frame));
    recordMarkReason_->clear();
  });
  canvas_->setExtractClicked([this](const CanvasFrameHit& hit) {
    acceptExtractFrame(hit.ok, hit.frame, QString::fromUtf8(hit.reason));
  });
  canvas_->setDropRequested([this](const QString& objectId, const QString& trackId, std::int64_t frame,
                                   const QString& error) {
    if (!error.isEmpty()) {
      explain(error.toStdString());
      return;
    }
    beginPlacement(objectId, trackId, frame);
  });
  auto* scroll = new QScrollArea(timeline);
  scroll->setObjectName("timelineScroll");
  scroll->setWidget(canvas_);
  scroll->setWidgetResizable(false);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setMinimumHeight(160);
  trackHeaders_ = new TrackHeaderColumn(timeline);
  auto* tracksRow = new QHBoxLayout();
  tracksRow->setContentsMargins(0, 0, 0, 0);
  tracksRow->setSpacing(0);
  tracksRow->addWidget(trackHeaders_);
  tracksRow->addWidget(scroll, 1);
  timelineLayout->addLayout(tracksRow, 1);
  navigator_ = new TimelineNavigator(canvas_, scroll, timeline);
  timelineLayout->addWidget(navigator_);
  connect(scroll->verticalScrollBar(), &QScrollBar::valueChanged, trackHeaders_, [this](int value) {
    trackHeaders_->setVerticalOffset(value);
  });

  rejection_ = new QLabel(timeline);
  rejection_->setObjectName("rejection");
  rejection_->setWordWrap(true);
  timelineLayout->addWidget(rejection_);

  auto* recordForm = menuForm(viewerMenuButton->menu());
  auto* recordLayout = qobject_cast<QVBoxLayout*>(recordForm->layout());
  auto* markUnit = new QLabel("sequence frames", recordForm);
  markUnit->setObjectName("recordMarkUnit");
  recordLayout->addWidget(markUnit);
  recordMarkFrame_ = new QLineEdit(recordForm);
  recordMarkFrame_->setObjectName("recordMarkFrame");
  recordMarkFrame_->setPlaceholderText("sequence frames");
  recordLayout->addWidget(recordMarkFrame_);
  auto* markButtons = new QGridLayout();
  auto addMarkButton = [this, recordForm, markButtons](const char* text, const char* name, int row, int column,
                                                       void (TimelineWorkspace::*method)()) {
    auto* button = new QPushButton(text, recordForm);
    button->setObjectName(name);
    markButtons->addWidget(button, row, column);
    connect(button, &QPushButton::clicked, this, method);
  };
  addMarkButton("Clear In", "recordClearIn", 0, 0, &TimelineWorkspace::clearRecordIn);
  addMarkButton("Clear Out", "recordClearOut", 0, 1, &TimelineWorkspace::clearRecordOut);
  addMarkButton("Clear Both", "recordClearBoth", 0, 2, &TimelineWorkspace::clearRecordBoth);
  recordLayout->addLayout(markButtons);
  recordMarkReadout_ = new QLabel("No sequence is selected.", recordForm);
  recordMarkReadout_->setObjectName("recordMarkReadout");
  recordMarkReadout_->setWordWrap(true);
  recordLayout->addWidget(recordMarkReadout_);
  recordMarkReason_ = new QLabel(recordForm);
  recordMarkReason_->setObjectName("recordMarkReason");
  recordMarkReason_->setWordWrap(true);
  recordLayout->addWidget(recordMarkReason_);
  addMenuForm(viewerMenuButton->menu(), recordForm);

  auto* editForm = menuForm(timelineMenuButton->menu());
  auto* editLayout = qobject_cast<QVBoxLayout*>(editForm->layout());
  inspector_ = new QLabel("No instance selected.", editForm);
  inspector_->setObjectName("inspector");
  inspector_->setWordWrap(true);
  inspector_->setAlignment(Qt::AlignTop);
  editLayout->addWidget(inspector_);
  instances_ = new QListWidget(editForm);
  instances_->setObjectName("instanceList");
  instances_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  instances_->setSelectionMode(QAbstractItemView::SingleSelection);
  instances_->setMinimumHeight(72);
  editLayout->addWidget(instances_);
  auto* moveRow = new QHBoxLayout();
  moveRow->addWidget(new QLabel("Move record start", editForm));
  moveStart_ = new QLineEdit(editForm);
  moveStart_->setObjectName("moveRecordStart");
  moveRow->addWidget(moveStart_);
  applyMoveButton_ = new QPushButton("Apply move", editForm);
  applyMoveButton_->setObjectName("applyMove");
  moveRow->addWidget(applyMoveButton_);
  editLayout->addLayout(moveRow);
  auto* trimRow = new QHBoxLayout();
  trimSourceStart_ = new QLineEdit(editForm);
  trimSourceEnd_ = new QLineEdit(editForm);
  trimRecordStart_ = new QLineEdit(editForm);
  trimRecordEnd_ = new QLineEdit(editForm);
  trimSourceStart_->setObjectName("trimSourceStart");
  trimSourceEnd_->setObjectName("trimSourceEnd");
  trimRecordStart_->setObjectName("trimRecordStart");
  trimRecordEnd_->setObjectName("trimRecordEnd");
  trimSourceStart_->setPlaceholderText("source start");
  trimSourceEnd_->setPlaceholderText("source end");
  trimRecordStart_->setPlaceholderText("record start");
  trimRecordEnd_->setPlaceholderText("record end");
  trimRow->addWidget(trimSourceStart_);
  trimRow->addWidget(trimSourceEnd_);
  trimRow->addWidget(trimRecordStart_);
  trimRow->addWidget(trimRecordEnd_);
  applyTrimButton_ = new QPushButton("Apply trim", editForm);
  applyTrimButton_->setObjectName("applyTrim");
  trimRow->addWidget(applyTrimButton_);
  editLayout->addLayout(trimRow);
  auto* undoRow = new QHBoxLayout();
  undoButton_ = new QPushButton("Undo", editForm);
  redoButton_ = new QPushButton("Redo", editForm);
  undoButton_->setObjectName("undoEdit");
  redoButton_->setObjectName("redoEdit");
  undoRow->addWidget(undoButton_);
  undoRow->addWidget(redoButton_);
  editLayout->addLayout(undoRow);
  auto* splitRow = new QHBoxLayout();
  splitRow->addWidget(new QLabel("Sequence frame", editForm));
  splitFrame_ = new QLineEdit(editForm);
  splitFrame_->setObjectName("splitFrame");
  splitFrame_->setPlaceholderText("sequence frame");
  splitFrameShown_ = new QLabel("sequence frame is not set", editForm);
  splitFrameShown_->setObjectName("splitFrameShown");
  splitRow->addWidget(splitFrame_);
  splitRow->addWidget(splitFrameShown_);
  editLayout->addLayout(splitRow);
  overwriteTrack_ = new QComboBox(editForm);
  overwriteTrack_->setObjectName("overwriteTrack");
  overwriteRate_ = new QComboBox(editForm);
  overwriteRate_->setObjectName("overwriteRate");
  overwriteRate_->addItem("Choose a source rate");
  const FrameRate rates[] = {{24, 1}, {25, 1}, {24000, 1001}, {30000, 1001}, {60000, 1001}};
  for (const FrameRate rate : rates) {
    const QString label = QString::number(rate.numerator) + "/" + QString::number(rate.denominator);
    overwriteRate_->addItem(label, label);
  }
  auto* overwriteRow = new QHBoxLayout();
  overwriteRow->addWidget(overwriteTrack_);
  overwriteRow->addWidget(overwriteRate_);
  editLayout->addLayout(overwriteRow);
  addMenuForm(timelineMenuButton->menu(), editForm);
  overwriteButton_ = overwriteButton;
  splitButton_ = splitButton;

  vertical->addWidget(upper);
  vertical->addWidget(timeline);
  vertical->setStretchFactor(0, 3);
  vertical->setStretchFactor(1, 2);
  vertical->setSizes({640, 420});

  connect(sequences_, &QListWidget::currentRowChanged, this, [this](int) { showSelectedSequence(); });
  connect(instances_, &QListWidget::currentRowChanged, this, [this](int) { showInspector(); });
  connect(applyMoveButton_, &QPushButton::clicked, this, &TimelineWorkspace::applyMove);
  connect(applyTrimButton_, &QPushButton::clicked, this, &TimelineWorkspace::applyTrim);
  connect(undoButton_, &QPushButton::clicked, this, &TimelineWorkspace::undoEdit);
  connect(redoButton_, &QPushButton::clicked, this, &TimelineWorkspace::redoEdit);
  connect(overwriteButton_, &QPushButton::clicked, this, &TimelineWorkspace::applyOverwrite);
  connect(splitFrame_, &QLineEdit::textChanged, this, [this](const QString& text) {
    const IntegerFrameParse parsed = parseIntegerFrame(text.toStdString());
    if (!parsed.ok) {
      splitFrameShown_->setText("sequence frame is not set");
      return;
    }
    splitFrameShown_->setText(QString::number(parsed.frame));
  });
  connect(splitButton_, &QPushButton::clicked, this, &TimelineWorkspace::applySplit);
  connect(liftButton, &QPushButton::clicked, this, &TimelineWorkspace::applyLift);
  resetRangeInteraction();
  if (rejection_ != nullptr) {
    rejection_->clear();
  }
  reload();
}

void TimelineWorkspace::reload() {
  const QString sequenceId = selectedSequenceId();
  const QString instanceId = selectedInstanceId();
  reloading_ = true;
  reloadSequences();
  reloading_ = false;
  if (!sequenceId.isEmpty()) {
    selectSequence(sequenceId);
  } else {
    showSelectedSequence();
  }
  if (!instanceId.isEmpty()) {
    selectInstance(instanceId);
  }
  syncUndo();
}

void TimelineWorkspace::reloadSequences() {
  sequences_->clear();
  if (!session_.isOpen()) {
    return;
  }
  auto sequences = session_.listSequences();
  if (!sequences.ok()) {
    explain(sequences.error.message);
    return;
  }
  for (const SequenceRecord& sequence : sequences.value) {
    const std::string label = sequence.name + "  " + std::to_string(sequence.rate.numerator) + "/" +
                              std::to_string(sequence.rate.denominator);
    auto* item = new QListWidgetItem(QString::fromStdString(label), sequences_);
    item->setData(Qt::UserRole, QString::fromStdString(sequence.sequenceId));
  }
}

void TimelineWorkspace::updateSequenceTab() {
  if (sequenceTab_ == nullptr) {
    return;
  }
  const QListWidgetItem* item = sequences_ == nullptr ? nullptr : sequences_->currentItem();
  if (!session_.isOpen() || item == nullptr) {
    sequenceTab_->setText("No sequence");
    return;
  }
  sequenceTab_->setText(item->text());
}

QHash<QString, QString> TimelineWorkspace::fileNamesByObject() const {
  QHash<QString, QString> names;
  if (!session_.isOpen()) {
    return names;
  }
  auto bins = session_.listBins();
  if (!bins.ok()) {
    return names;
  }
  for (const BinRecord& bin : bins.value) {
    auto assets = session_.listBinAssets(bin.binId);
    if (!assets.ok()) {
      continue;
    }
    for (const BinAssetFact& asset : assets.value) {
      names.insert(QString::fromStdString(asset.objectId), QString::fromStdString(asset.fileName));
    }
  }
  return names;
}

void TimelineWorkspace::showSelectedSequence() {
  if (reloading_) {
    return;
  }
  instances_->clear();
  std::vector<TimelineCanvas::Row> rows;
  if (!session_.isOpen() || selectedSequenceId().isEmpty()) {
    canvas_->setRows({});
    syncTrackChrome();
    rulerCaption_->setText("No sequence selected. Drop-frame flag absent.");
    if (timelineClock_ != nullptr) {
      timelineClock_->setText("—");
    }
    updateSequenceTab();
    showInspector();
    refreshRecordMarks();
    refreshOverwriteTracks();
    return;
  }
  const std::string sequenceId = selectedSequenceId().toStdString();
  auto sequence = session_.readSequence(sequenceId);
  if (!sequence.ok()) {
    explain(sequence.error.message);
    refreshRecordMarks();
    refreshOverwriteTracks();
    return;
  }
  const std::string label = formatFrameLabel(0, sequence.value.rate);
  rulerCaption_->setText(QString::fromStdString(
      "Sequence frame 0 label " + label + ". Drop-frame flag absent. Pixels are not stored."));
  if (timelineClock_ != nullptr) {
    timelineClock_->setText(QString::fromStdString(label.empty() ? std::string("frame 0") : label));
  }
  updateSequenceTab();
  auto tracks = session_.listTracks(sequenceId);
  if (!tracks.ok()) {
    explain(tracks.error.message);
    refreshRecordMarks();
    refreshOverwriteTracks();
    return;
  }
  for (const TrackRecord& track : tracks.value) {
    TimelineCanvas::Row row;
    row.trackId = QString::fromStdString(track.trackId);
    row.ordinal = track.ordinal;
    auto instances = session_.listInstances(track.trackId);
    if (!instances.ok()) {
      explain(instances.error.message);
      refreshRecordMarks();
      refreshOverwriteTracks();
      return;
    }
    const QHash<QString, QString> names = fileNamesByObject();
    for (const TimelineInstance& instance : instances.value) {
      const std::int64_t duration = instance.recordEnd - instance.recordStart;
      const std::string text = "track " + std::to_string(track.ordinal) + " start " +
                               std::to_string(instance.recordStart) + " duration " + std::to_string(duration);
      auto* item = new QListWidgetItem(QString::fromStdString(text), instances_);
      item->setData(Qt::UserRole, QString::fromStdString(instance.instanceId));
      TimelineCanvas::Block block;
      block.instanceId = QString::fromStdString(instance.instanceId);
      block.title = names.value(QString::fromStdString(instance.masterObjectId));
      block.recordStart = instance.recordStart;
      block.recordEnd = instance.recordEnd;
      block.selected = block.instanceId == selectedInstanceId();
      row.blocks.push_back(block);
    }
    rows.push_back(std::move(row));
  }
  canvas_->setRows(std::move(rows));
  syncTrackChrome();
  showInspector();
  refreshRecordMarks();
  refreshOverwriteTracks();
}

void TimelineWorkspace::refreshCanvas() {
  std::vector<TimelineCanvas::Row> rows;
  if (!session_.isOpen() || selectedSequenceId().isEmpty()) {
    canvas_->setRows({});
    syncTrackChrome();
    return;
  }
  auto tracks = session_.listTracks(selectedSequenceId().toStdString());
  if (!tracks.ok()) {
    return;
  }
  for (const TrackRecord& track : tracks.value) {
    TimelineCanvas::Row row;
    row.trackId = QString::fromStdString(track.trackId);
    row.ordinal = track.ordinal;
    auto instances = session_.listInstances(track.trackId);
    if (!instances.ok()) {
      return;
    }
    const QHash<QString, QString> names = fileNamesByObject();
    for (const TimelineInstance& instance : instances.value) {
      TimelineCanvas::Block block;
      block.instanceId = QString::fromStdString(instance.instanceId);
      block.title = names.value(QString::fromStdString(instance.masterObjectId));
      block.recordStart = instance.recordStart;
      block.recordEnd = instance.recordEnd;
      block.selected = block.instanceId == selectedInstanceId();
      row.blocks.push_back(block);
    }
    rows.push_back(std::move(row));
  }
  canvas_->setRows(std::move(rows));
  syncTrackChrome();
}

void TimelineWorkspace::syncTrackChrome() {
  if (trackHeaders_ == nullptr || canvas_ == nullptr) {
    return;
  }
  std::vector<TrackHeaderColumn::Track> tracks;
  tracks.reserve(canvas_->rows().size());
  for (const TimelineCanvas::Row& row : canvas_->rows()) {
    tracks.push_back(TrackHeaderColumn::Track{row.trackId, row.ordinal});
  }
  trackHeaders_->setTracks(tracks);
  if (navigator_ != nullptr) {
    navigator_->update();
  }
}

void TimelineWorkspace::clearEditFields() {
  moveStart_->clear();
  trimSourceStart_->clear();
  trimSourceEnd_->clear();
  trimRecordStart_->clear();
  trimRecordEnd_->clear();
}

void TimelineWorkspace::showInspector() {
  if (!session_.isOpen() || selectedInstanceId().isEmpty()) {
    inspector_->setText("No instance selected.");
    clearEditFields();
    refreshCanvas();
    syncUndo();
    emit selectionChanged();
    return;
  }
  auto instance = session_.readInstance(selectedInstanceId().toStdString());
  if (!instance.ok()) {
    inspector_->setText("No instance selected.");
    clearEditFields();
    refreshCanvas();
    syncUndo();
    emit selectionChanged();
    return;
  }
  const TimelineInstance& row = instance.value;
  const std::int64_t duration = row.recordEnd - row.recordStart;
  std::string text = "instance " + row.instanceId + "\ntrack " + row.trackId + "\nrecord [" +
                     std::to_string(row.recordStart) + "," + std::to_string(row.recordEnd) + ") duration " +
                     std::to_string(duration) + " frames\nsource [" + std::to_string(row.sourceStart) + "," +
                     std::to_string(row.sourceEnd) + ") " + std::to_string(row.sourceRate.numerator) + "/" +
                     std::to_string(row.sourceRate.denominator) + "\nmedia " + row.mediaId + "\nmaster " +
                     row.masterObjectId;
  inspector_->setText(QString::fromStdString(text));
  fillFieldsFromSelection();
  refreshCanvas();
  syncUndo();
  emit selectionChanged();
}

void TimelineWorkspace::fillFieldsFromSelection() {
  auto instance = session_.readInstance(selectedInstanceId().toStdString());
  if (!instance.ok()) {
    return;
  }
  moveStart_->setText(QString::number(instance.value.recordStart));
  trimSourceStart_->setText(QString::number(instance.value.sourceStart));
  trimSourceEnd_->setText(QString::number(instance.value.sourceEnd));
  trimRecordStart_->setText(QString::number(instance.value.recordStart));
  trimRecordEnd_->setText(QString::number(instance.value.recordEnd));
}

void TimelineWorkspace::explain(const std::string& message) {
  rejection_->setText(QString::fromStdString(message));
}

QString TimelineWorkspace::selectedSequenceId() const {
  const QListWidgetItem* item = sequences_->currentItem();
  if (item == nullptr) {
    return {};
  }
  return item->data(Qt::UserRole).toString();
}

QStringList TimelineWorkspace::trackIdsInOrder() const {
  QStringList ids;
  if (!session_.isOpen() || selectedSequenceId().isEmpty()) {
    return ids;
  }
  auto tracks = session_.listTracks(selectedSequenceId().toStdString());
  if (!tracks.ok()) {
    return ids;
  }
  for (const TrackRecord& track : tracks.value) {
    ids.push_back(QString::fromStdString(track.trackId));
  }
  return ids;
}

QString TimelineWorkspace::selectedInstanceId() const {
  const QListWidgetItem* item = instances_->currentItem();
  if (item == nullptr) {
    return {};
  }
  return item->data(Qt::UserRole).toString();
}

QString TimelineWorkspace::inspectorText() const {
  return inspector_->text();
}

QString TimelineWorkspace::rejectionText() const {
  return rejection_->text();
}

QString TimelineWorkspace::rulerCaption() const {
  return rulerCaption_->text();
}

bool TimelineWorkspace::selectSequence(const QString& sequenceId) {
  for (int index = 0; index < sequences_->count(); ++index) {
    if (sequences_->item(index)->data(Qt::UserRole).toString() == sequenceId) {
      sequences_->setCurrentRow(index);
      return true;
    }
  }
  return false;
}

bool TimelineWorkspace::selectInstance(const QString& instanceId) {
  for (int index = 0; index < instances_->count(); ++index) {
    if (instances_->item(index)->data(Qt::UserRole).toString() == instanceId) {
      instances_->setCurrentRow(index);
      return true;
    }
  }
  return false;
}

void TimelineWorkspace::beginPlacement(const QString& objectId,
                                        const QString& trackId,
                                        std::int64_t recordFrame) {
  if (!session_.isOpen() || selectedSequenceId().isEmpty()) {
    explain("no sequence is selected");
    return;
  }
  PlacementDialog dialog(session_, this);
  dialog.setRequest(selectedSequenceId(), objectId, trackId, recordFrame);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }
  const QString instanceId = dialog.createdInstanceId();
  reload();
  selectInstance(instanceId);
  explain("");
}

void TimelineWorkspace::clearInstanceSelection() {
  if (instances_->currentRow() >= 0) {
    instances_->setCurrentRow(-1);
    return;
  }
  showInspector();
}

bool TimelineWorkspace::deliverTimelineEvent(QEvent* event) { return canvas_->deliver(event); }

QPoint TimelineWorkspace::blockCenter(const QString& instanceId) const {
  return canvas_->blockCenter(instanceId);
}

bool TimelineWorkspace::blockIsSelected(const QString& instanceId) const {
  return canvas_->blockIsSelected(instanceId);
}

void TimelineWorkspace::syncUndo() {
  if (undoButton_ != nullptr) {
    undoButton_->setEnabled(session_.canUndo());
  }
  if (redoButton_ != nullptr) {
    redoButton_->setEnabled(session_.canRedo());
  }
  emit historyChanged();
}

void TimelineWorkspace::setMoveRecordStartText(const QString& text) {
  moveStart_->setText(text);
}

void TimelineWorkspace::setTrimText(const QString& sourceStart,
                                    const QString& sourceEnd,
                                    const QString& recordStart,
                                    const QString& recordEnd) {
  trimSourceStart_->setText(sourceStart);
  trimSourceEnd_->setText(sourceEnd);
  trimRecordStart_->setText(recordStart);
  trimRecordEnd_->setText(recordEnd);
}

void TimelineWorkspace::applyMove() {
  if (selectedInstanceId().isEmpty()) {
    explain("no instance is selected");
    return;
  }
  const IntegerFrameParse parsed = parseIntegerFrame(moveStart_->text().toStdString());
  if (!parsed.ok) {
    explain(parsed.reason);
    return;
  }
  const QString instanceId = selectedInstanceId();
  auto moved = session_.moveInstance(instanceId.toStdString(), parsed.frame);
  if (!moved.ok()) {
    explain(moved.error.message);
    reload();
    selectInstance(instanceId);
    return;
  }
  explain("");
  reload();
  selectInstance(instanceId);
  syncUndo();
}

void TimelineWorkspace::applyTrim() {
  if (selectedInstanceId().isEmpty()) {
    explain("no instance is selected");
    return;
  }
  const IntegerFrameParse sourceStart = parseIntegerFrame(trimSourceStart_->text().toStdString());
  const IntegerFrameParse sourceEnd = parseIntegerFrame(trimSourceEnd_->text().toStdString());
  const IntegerFrameParse recordStart = parseIntegerFrame(trimRecordStart_->text().toStdString());
  const IntegerFrameParse recordEnd = parseIntegerFrame(trimRecordEnd_->text().toStdString());
  if (!sourceStart.ok || !sourceEnd.ok || !recordStart.ok || !recordEnd.ok) {
    explain("a frame must be a non-negative integer");
    return;
  }
  const QString instanceId = selectedInstanceId();
  auto trimmed = session_.trimInstance(instanceId.toStdString(), sourceStart.frame, sourceEnd.frame, recordStart.frame,
                                       recordEnd.frame);
  if (!trimmed.ok()) {
    explain(trimmed.error.message);
    reload();
    selectInstance(instanceId);
    return;
  }
  explain("");
  reload();
  selectInstance(instanceId);
  syncUndo();
}

void TimelineWorkspace::applySplit() {
  if (selectedInstanceId().isEmpty()) {
    explain("no instance is selected");
    return;
  }
  const IntegerFrameParse parsed = parseIntegerFrame(splitFrame_->text().toStdString());
  if (!parsed.ok) {
    explain(parsed.reason);
    return;
  }
  const QString instanceId = selectedInstanceId();
  auto split = session_.split(instanceId.toStdString(), parsed.frame);
  if (!split.ok()) {
    explain(split.error.message);
    reload();
    selectInstance(instanceId);
    return;
  }
  explain("");
  reload();
  selectInstance(instanceId);
  syncUndo();
}

void TimelineWorkspace::applyLift() {
  if (selectedInstanceId().isEmpty()) {
    explain("no instance is selected");
    return;
  }
  const QString instanceId = selectedInstanceId();
  auto lifted = session_.lift(instanceId.toStdString());
  if (!lifted.ok()) {
    explain(lifted.error.message);
    reload();
    selectInstance(instanceId);
    return;
  }
  explain("");
  reload();
  syncUndo();
}

void TimelineWorkspace::undoEdit() {
  auto undone = session_.undo();
  if (!undone.ok()) {
    explain(undone.error.message);
    return;
  }
  explain("");
  reload();
  syncUndo();
}

void TimelineWorkspace::setOverwriteSource(const QString& objectId, const QString& representationId) {
  overwriteObjectId_ = objectId;
  overwriteRepresentationId_ = representationId;
}

void TimelineWorkspace::refreshOverwriteTracks() {
  if (overwriteTrack_ == nullptr) {
    return;
  }
  const QString previous = overwriteTrack_->currentData().toString();
  overwriteTrack_->blockSignals(true);
  overwriteTrack_->clear();
  overwriteTrack_->addItem("Choose a track", QString());
  if (session_.isOpen() && !selectedSequenceId().isEmpty()) {
    auto tracks = session_.listTracks(selectedSequenceId().toStdString());
    if (tracks.ok()) {
      for (const TrackRecord& track : tracks.value) {
        overwriteTrack_->addItem(QString::fromStdString("track " + std::to_string(track.ordinal)),
                                 QString::fromStdString(track.trackId));
      }
    }
  }
  int restore = 0;
  for (int index = 0; index < overwriteTrack_->count(); ++index) {
    if (!previous.isEmpty() && overwriteTrack_->itemData(index).toString() == previous) {
      restore = index;
    }
  }
  overwriteTrack_->setCurrentIndex(restore);
  overwriteTrack_->blockSignals(false);
}

void TimelineWorkspace::applyOverwrite() {
  FrameRate rate;
  if (overwriteRate_ != nullptr && overwriteRate_->currentIndex() > 0) {
    const QString label = overwriteRate_->currentData().toString();
    const int slash = label.indexOf('/');
    if (slash > 0) {
      rate.numerator = label.left(slash).toLongLong();
      rate.denominator = label.mid(slash + 1).toLongLong();
    }
  }
  const QString trackId = overwriteTrack_ == nullptr ? QString() : overwriteTrack_->currentData().toString();
  auto wrote = session_.overwrite(selectedSequenceId().toStdString(), trackId.toStdString(),
                                  overwriteObjectId_.toStdString(), overwriteRepresentationId_.toStdString(), rate);
  if (!wrote.ok()) {
    explain(wrote.error.message);
    return;
  }
  explain("");
  reload();
  selectInstance(QString::fromStdString(wrote.value.instanceId));
}

void TimelineWorkspace::redoEdit() {
  auto redone = session_.redo();
  if (!redone.ok()) {
    explain(redone.error.message);
    return;
  }
  explain("");
  reload();
  syncUndo();
}

namespace {

QString formatRecordMarks(const std::string& sequenceId, const MarkPoints& marks) {
  QString text = QString::fromStdString("Sequence " + sequenceId + "\n");
  if (marks.markIn.has_value()) {
    text += QString::fromStdString("Mark In: " + std::to_string(*marks.markIn) + " sequence frames\n");
  } else {
    text += "Mark In: none\n";
  }
  if (marks.markOut.has_value()) {
    text += QString::fromStdString("Mark Out: " + std::to_string(*marks.markOut) + " sequence frames\n");
  } else {
    text += "Mark Out: none\n";
  }
  text += "Sequence frames. The sequence rate is not a source rate.\n";
  if (marks.markIn.has_value() && marks.markOut.has_value()) {
    text += "Points only. Displayed inclusive OUT is not settled, so this pair is not a half-open edit range.";
  } else if (marks.markIn.has_value() || marks.markOut.has_value()) {
    text += "A single point is not an edit range.";
  } else {
    text += "No record marks are stored.";
  }
  return text;
}

}  // namespace

QString TimelineWorkspace::recordMarkReadoutText() const { return recordMarkReadout_->text(); }

std::int64_t TimelineWorkspace::playheadFrame() const {
  return canvas_ == nullptr ? 0 : canvas_->playheadFrame();
}

void TimelineWorkspace::setPlayheadFrame(std::int64_t frame) {
  if (canvas_ == nullptr) {
    return;
  }
  const std::int64_t next = frame < 0 ? 0 : frame;
  if (next == canvas_->playheadFrame()) {
    return;
  }
  canvas_->setPlayheadFrame(next);
  emit playheadChanged();
}

std::int64_t TimelineWorkspace::contentEndFrame() const {
  if (!session_.isOpen() || selectedSequenceId().isEmpty()) {
    return 0;
  }
  auto tracks = session_.listTracks(selectedSequenceId().toStdString());
  if (!tracks.ok()) {
    return 0;
  }
  std::int64_t end = 0;
  for (const TrackRecord& track : tracks.value) {
    auto rows = session_.listInstances(track.trackId);
    if (!rows.ok()) {
      continue;
    }
    for (const TimelineInstance& row : rows.value) {
      if (row.recordEnd > end) {
        end = row.recordEnd;
      }
    }
  }
  return end;
}

PlayheadPicture TimelineWorkspace::pictureAtPlayhead() const {
  PlayheadPicture result;
  result.note = "No picture covers this playhead.";
  if (!session_.isOpen() || selectedSequenceId().isEmpty()) {
    result.note = "No sequence is selected.";
    return result;
  }
  auto tracks = session_.listTracks(selectedSequenceId().toStdString());
  if (!tracks.ok()) {
    result.note = QString::fromStdString(tracks.error.message);
    return result;
  }
  const std::int64_t playhead = playheadFrame();
  int bestOrdinal = std::numeric_limits<int>::min();
  TimelineInstance best;
  bool found = false;
  for (const TrackRecord& track : tracks.value) {
    if (track.ordinal % 2 != 0) {
      continue;
    }
    auto rows = session_.listInstances(track.trackId);
    if (!rows.ok()) {
      continue;
    }
    for (const TimelineInstance& row : rows.value) {
      if (playhead < row.recordStart || playhead >= row.recordEnd) {
        continue;
      }
      if (!found || track.ordinal >= bestOrdinal) {
        found = true;
        bestOrdinal = track.ordinal;
        best = row;
      }
    }
  }
  if (!found) {
    return result;
  }
  const std::int64_t delta = playhead - best.recordStart;
  if (best.sourceStart < 0 || delta < 0 ||
      best.sourceStart > std::numeric_limits<std::int64_t>::max() - delta) {
    result.note = "The stored source offset does not fit a picture index.";
    return result;
  }
  std::string mediaId = best.mediaId;
  if (mediaId.empty()) {
    auto clip = session_.readMasterClip(best.masterObjectId);
    if (clip.ok()) {
      mediaId = clip.value.mediaId;
    }
  }
  auto reps = session_.listRepresentations(mediaId);
  if (!reps.ok()) {
    result.note = QString::fromStdString(reps.error.message);
    return result;
  }
  std::string path;
  for (const Representation& rep : reps.value) {
    if (rep.role == RepresentationRole::Original && !rep.recordedPath.empty()) {
      path = rep.recordedPath;
      break;
    }
    if (path.empty()) {
      path = rep.recordedPath;
    }
  }
  if (path.empty()) {
    result.note = "The clip has no recorded path.";
    return result;
  }
  result.covered = true;
  result.path = QString::fromStdString(path);
  result.mediaIndex = best.sourceStart + delta;
  result.note.clear();
  return result;
}

QStringList TimelineWorkspace::extractParticipantIds() const {
  return trackHeaders_ == nullptr ? QStringList() : trackHeaders_->participantIds();
}

void TimelineWorkspace::refreshRecordMarks() {
  const QString sequenceId = selectedSequenceId();
  bool sequenceChanged = false;
  if (sequenceId != displayedSequenceId_) {
    displayedSequenceId_ = sequenceId;
    recordMarkFrame_->clear();
    recordMarkReason_->clear();
    setPlayheadFrame(0);
    sequenceChanged = true;
  }
  refreshExtractTracks(sequenceChanged);
  if (!session_.isOpen() || sequenceId.isEmpty()) {
    recordMarkReadout_->setText("No sequence is selected.");
    canvas_->setMarks(std::nullopt, std::nullopt);
    emit recordMarksChanged();
    return;
  }
  auto marks = session_.recordMarks(sequenceId.toStdString());
  if (!marks.ok()) {
    recordMarkReadout_->setText(QString::fromStdString(marks.error.message));
    canvas_->setMarks(std::nullopt, std::nullopt);
    emit recordMarksChanged();
    return;
  }
  recordMarkReadout_->setText(formatRecordMarks(sequenceId.toStdString(), marks.value));
  canvas_->setMarks(marks.value.markIn, marks.value.markOut);
  emit recordMarksChanged();
}

void TimelineWorkspace::resetRangeInteraction() {
  extractAwaitingEnd_ = false;
  if (extractStart_ != nullptr) {
    extractStart_->clear();
  }
  if (extractEnd_ != nullptr) {
    extractEnd_->clear();
  }
  if (extractRangeReadout_ != nullptr) {
    extractRangeReadout_->setText("No record range.");
  }
  if (canvas_ != nullptr) {
    canvas_->setExtractRange(std::nullopt, std::nullopt);
    canvas_->setExtractRangeMode(false);
  }
  if (extractRangeMode_ != nullptr) {
    const QSignalBlocker blocker(extractRangeMode_);
    extractRangeMode_->setChecked(false);
  }
  session_.clearExtractRange();
}

void TimelineWorkspace::updateVisibleEnd(const std::vector<TimelineInstance>& rows) {
  if (visibleEnd_ == nullptr) {
    return;
  }
  bool any = false;
  std::int64_t end = 0;
  for (const TimelineInstance& row : rows) {
    if (!any || row.recordEnd > end) {
      end = row.recordEnd;
      any = true;
    }
  }
  visibleEnd_->setText(any ? QString("Visible end %1").arg(end) : QString("Visible end none"));
}

void TimelineWorkspace::refreshExtractTracks(bool sequenceChanged) {
  if (extractStart_ == nullptr || extractEnd_ == nullptr) {
    return;
  }
  if (sequenceChanged) {
    resetRangeInteraction();
    if (rejection_ != nullptr) {
      rejection_->clear();
    }
    if (trackHeaders_ != nullptr) {
      trackHeaders_->clearParticipants();
    }
    syncUndo();
  }
  std::vector<TimelineInstance> rows;
  if (!session_.isOpen() || selectedSequenceId().isEmpty()) {
    updateVisibleEnd(rows);
    return;
  }
  auto tracks = session_.listTracks(selectedSequenceId().toStdString());
  if (!tracks.ok()) {
    updateVisibleEnd(rows);
    return;
  }
  for (const TrackRecord& track : tracks.value) {
    auto instances = session_.listInstances(track.trackId);
    if (instances.ok()) {
      rows.insert(rows.end(), instances.value.begin(), instances.value.end());
    }
  }
  updateVisibleEnd(rows);
}

void TimelineWorkspace::publishExtractRange() {
  if (extractStart_ == nullptr || extractEnd_ == nullptr) {
    return;
  }
  const QString sequenceId = selectedSequenceId();
  bool startOk = false;
  bool endOk = false;
  const qlonglong start = extractStart_->text().trimmed().toLongLong(&startOk);
  const qlonglong end = extractEnd_->text().trimmed().toLongLong(&endOk);
  if (sequenceId.isEmpty() || !startOk || !endOk) {
    extractRangeReadout_->setText("Start and end are integer sequence frames. End is exclusive.");
    return;
  }
  auto stored = session_.setExtractRange(sequenceId.toStdString(), start, end);
  if (!stored.ok()) {
    extractRangeReadout_->setText(QString::fromStdString(stored.error.message));
    explain(stored.error.message);
    std::string storedSequence;
    std::int64_t storedStart = 0;
    std::int64_t storedEnd = 0;
    if (session_.extractRange(&storedSequence, &storedStart, &storedEnd) &&
        QString::fromStdString(storedSequence) == sequenceId) {
      canvas_->setExtractRange(storedStart, storedEnd);
    }
    return;
  }
  extractRangeReadout_->setText(
      QString("Start %1 inclusive. End %2 exclusive. D %3.").arg(start).arg(end).arg(end - start));
  canvas_->setExtractRange(start, end);
}

void TimelineWorkspace::acceptExtractFrame(bool ok, std::int64_t frame, const QString& reason) {
  if (!ok) {
    explain(reason.toStdString());
    extractRangeReadout_->setText(reason);
    return;
  }
  if (!extractAwaitingEnd_ || extractStart_->text().trimmed().isEmpty()) {
    extractStart_->setText(QString::number(frame));
    extractEnd_->clear();
    extractAwaitingEnd_ = true;
    extractRangeReadout_->setText(QString("Start %1 inclusive. End not set.").arg(frame));
    return;
  }
  extractEnd_->setText(QString::number(frame));
  extractAwaitingEnd_ = false;
  publishExtractRange();
}

void TimelineWorkspace::applyExtract() {
  const QString sequenceId = selectedSequenceId();
  if (sequenceId.isEmpty()) {
    explain("No sequence is selected.");
    return;
  }
  bool startOk = false;
  bool endOk = false;
  const qlonglong start = extractStart_->text().trimmed().toLongLong(&startOk);
  const qlonglong end = extractEnd_->text().trimmed().toLongLong(&endOk);
  if (!startOk || !endOk) {
    explain("The record range needs two integer sequence frames.");
    return;
  }
  std::vector<std::string> tracks;
  const QStringList chosen = extractParticipantIds();
  tracks.reserve(static_cast<std::size_t>(chosen.size()));
  for (const QString& trackId : chosen) {
    tracks.push_back(trackId.toStdString());
  }
  session_.setExtractRange(sequenceId.toStdString(), start, end);
  auto result = session_.extract(sequenceId.toStdString(), tracks, start, end);
  if (!result.ok()) {
    explain(result.error.message);
    publishExtractRange();
    extractRangeReadout_->setText(QString::fromStdString(result.error.message));
    syncUndo();
    return;
  }
  extractStart_->clear();
  extractEnd_->clear();
  extractAwaitingEnd_ = false;
  extractRangeReadout_->setText("Extract closed the interval. The record range was cleared.");
  canvas_->setExtractRange(std::nullopt, std::nullopt);
  explain("");
  showSelectedSequence();
  syncUndo();
}

void TimelineWorkspace::markRecordIn() { applyRecordPoint(true); }

void TimelineWorkspace::markRecordOut() { applyRecordPoint(false); }

void TimelineWorkspace::clearRecordIn() { clearRecordPoint(true, false); }

void TimelineWorkspace::clearRecordOut() { clearRecordPoint(false, true); }

void TimelineWorkspace::clearRecordBoth() { clearRecordPoint(true, true); }

void TimelineWorkspace::applyRecordPoint(bool isIn) {
  const QString sequenceId = selectedSequenceId();
  refreshRecordMarks();
  if (!session_.isOpen() || sequenceId.isEmpty()) {
    recordMarkReason_->setText("the sequence is not in this project");
    return;
  }
  const IntegerFrameParse parsed = parseIntegerFrame(recordMarkFrame_->text().toStdString());
  if (!parsed.ok) {
    recordMarkReason_->setText(QString::fromUtf8(parsed.reason));
    return;
  }
  const auto result = isIn ? session_.setRecordMarkIn(sequenceId.toStdString(), parsed.frame)
                           : session_.setRecordMarkOut(sequenceId.toStdString(), parsed.frame);
  refreshRecordMarks();
  recordMarkReason_->setText(result.ok() ? QString() : QString::fromStdString(result.error.message));
}

void TimelineWorkspace::clearRecordPoint(bool clearIn, bool clearOut) {
  const QString sequenceId = selectedSequenceId();
  refreshRecordMarks();
  if (!session_.isOpen() || sequenceId.isEmpty()) {
    recordMarkReason_->setText("the sequence is not in this project");
    return;
  }
  const std::string id = sequenceId.toStdString();
  const Result<void> result = clearIn && clearOut ? session_.clearRecordMarks(id)
                               : clearIn          ? session_.clearRecordMarkIn(id)
                                                  : session_.clearRecordMarkOut(id);
  refreshRecordMarks();
  recordMarkReason_->setText(result.ok() ? QString() : QString::fromStdString(result.error.message));
}

}  // namespace aether
