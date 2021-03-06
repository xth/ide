/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef QMT_OBJECTITEM_H
#define QMT_OBJECTITEM_H

#include <QGraphicsItem>

#include "qmt/diagram_scene/capabilities/intersectionable.h"
#include "qmt/diagram_scene/capabilities/resizable.h"
#include "qmt/diagram_scene/capabilities/moveable.h"
#include "qmt/diagram_scene/capabilities/selectable.h"
#include "qmt/diagram_scene/capabilities/latchable.h"
#include "qmt/diagram_scene/capabilities/alignable.h"
#include "qmt/diagram_scene/capabilities/editable.h"

#include "qmt/stereotype/stereotypeicon.h"

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace qmt {

class DObject;
class DiagramSceneModel;
class StereotypesItem;
class CustomIconItem;
class EditableTextItem;
class RectangularSelectionItem;
class AlignButtonsItem;
class Style;

// typical z-values for graphic items
static const int SHAPE_ZVALUE = -100; // the filled background of the shape
static const int SHAPE_DETAILS_ZVALUE = -90; // any details to the shape (e.g. extra lines in shape)
static const int SELECTION_MARKER_ZVALUE = 100;

class ObjectItem :
        public QGraphicsItem,
        public IIntersectionable,
        public IResizable,
        public IMoveable,
        public ISelectable,
        public ILatchable,
        public IAlignable,
        public IEditable
{
protected:
    enum ResizeFlags {
        ResizeUnlocked,
        ResizeLockedSize,
        ResizeLockedWidth,
        ResizeLockedHeight,
        ResizeLockedRatio
    };

public:
    ObjectItem(DObject *object, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent = 0);
    ~ObjectItem() override;

    DObject *object() const { return m_object; }
    DiagramSceneModel *diagramSceneModel() const { return m_diagramSceneModel; }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    virtual void update() = 0;

    QPointF pos() const override;
    QRectF rect() const override;
    QSizeF minimumSize() const override = 0;
    void setPosAndRect(const QPointF &originalPos, const QRectF &originalRect,
                       const QPointF &topLeftDelta, const QPointF &bottomRightDelta) override;
    void alignItemSizeToRaster(Side adjustHorizontalSide, Side adjustVerticalSide,
                               double rasterWidth, double rasterHeight) override;

    void moveDelta(const QPointF &delta) override;
    void alignItemPositionToRaster(double rasterWidth, double rasterHeight) override;

    bool isSecondarySelected() const override;
    void setSecondarySelected(bool secondarySelected) override;
    bool isFocusSelected() const override;
    void setFocusSelected(bool focusSelected) override;

    Action horizontalLatchAction() const override;
    Action verticalLatchAction() const override;
    QList<Latch> horizontalLatches(Action action, bool grabbedItem) const override;
    QList<Latch> verticalLatches(Action action, bool grabbedItem) const override;

    void align(AlignType alignType, const QString &identifier) override;

    bool isEditable() const override;
    void edit() override;

protected:
    void updateStereotypeIconDisplay();
    QString stereotypeIconId() const { return m_stereotypeIconId; }
    QString shapeIconId() const { return m_shapeIconId; }
    StereotypeIcon::Display stereotypeIconDisplay() const { return m_stereotypeIconDisplay; }
    void updateStereotypes(const QString &stereotypeIconId,
                           StereotypeIcon::Display stereotypeDisplay, const Style *style);
    StereotypesItem *stereotypesItem() const { return m_stereotypes; }
    CustomIconItem *stereotypeIconItem() const { return m_stereotypeIcon; }
    QSizeF stereotypeIconMinimumSize(const StereotypeIcon &stereotypeIcon, qreal minimumWidth,
                                     qreal minimumHeight) const;
    void updateNameItem(const Style *style);
    EditableTextItem *nameItem() const { return m_nameItem; }
    virtual QString buildDisplayName() const;
    virtual void setFromDisplayName(const QString &displayName);
    void setObjectName(const QString &objectName);

    void updateDepth();
    void updateSelectionMarker(CustomIconItem *customIconItem);
    void updateSelectionMarker(ResizeFlags resizeFlags);
    void updateSelectionMarkerGeometry(const QRectF &objectRect);
    void updateAlignmentButtons();
    void updateAlignmentButtonsGeometry(const QRectF &objectRect);

    IAlignable::AlignType translateLatchTypeToAlignType(ILatchable::LatchType latchType);
    const Style *adaptedStyle(const QString &stereotypeIconId);
    bool showContext() const;

    virtual bool extendContextMenu(QMenu *menu);
    virtual bool handleSelectedContextMenuAction(QAction *action);

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    QSizeF minimumSize(const QSet<QGraphicsItem *> &items) const;

    DObject *m_object = 0;
    DiagramSceneModel *m_diagramSceneModel = 0;
    bool m_isSecondarySelected = false;
    bool m_isFocusSelected = false;
    QString m_stereotypeIconId;
    QString m_shapeIconId;
    StereotypeIcon::Display m_stereotypeIconDisplay = StereotypeIcon::DisplayLabel;
    StereotypesItem *m_stereotypes = 0;
    CustomIconItem *m_stereotypeIcon = 0;
    EditableTextItem *m_nameItem = 0;
    RectangularSelectionItem *m_selectionMarker = 0;
    AlignButtonsItem *m_horizontalAlignButtons = 0;
    AlignButtonsItem *m_verticalAlignButtons = 0;
};

} // namespace qmt

#endif // QMT_OBJECTITEM_H
