/*
 Copyright (C) 2010-2017 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "CellLayout.h"

#include "Ensure.h"
#include "Macros.h"

#include <algorithm>
#include <cassert>

namespace TrenchBroom {
    namespace View {
        float LayoutBounds::left() const {
            return x;
        }

        float LayoutBounds::top() const {
            return y;
        }

        float LayoutBounds::right() const {
            return x + width;
        }

        float LayoutBounds::bottom() const {
            return y + height;
        }

        bool LayoutBounds::containsPoint(const float pointX, const float pointY) const {
            return pointX >= left() && pointX <= right() && pointY >= top() && pointY <= bottom();
        }

        bool LayoutBounds::intersectsY(const float rangeY, const float rangeHeight) const {
            return bottom() >= rangeY && top() <= rangeY + rangeHeight ;
        }

        LayoutCell::LayoutCell(const float x, const float y,
                    const float itemWidth, const float itemHeight,
                    const float titleWidth, const float titleHeight,
                    const float titleMargin,
                    const float maxUpScale,
                    const float minWidth, const float maxWidth,
                    const float minHeight, const float maxHeight) :
        m_x{x},
        m_y{y},
        m_itemWidth{itemWidth},
        m_itemHeight{itemHeight},
        m_titleWidth{titleWidth},
        m_titleHeight{titleHeight},
        m_titleMargin{titleMargin} {
            doLayout(maxUpScale, minWidth, maxWidth, minHeight, maxHeight);
        }

        std::any& LayoutCell::item() {
            return m_item;
        }

        const std::any& LayoutCell::item() const {
            return m_item;
        }

        void LayoutCell::setItem(std::any item) {
            m_item = std::move(item);
        }

        float LayoutCell::scale() const {
            return m_scale;
        }

        const LayoutBounds& LayoutCell::bounds() const {
            return cellBounds();
        }

        const LayoutBounds& LayoutCell::cellBounds() const {
            return m_cellBounds;
        }

        const LayoutBounds& LayoutCell::titleBounds() const {
            return m_titleBounds;
        }

        const LayoutBounds& LayoutCell::itemBounds() const {
            return m_itemBounds;
        }

        bool LayoutCell::hitTest(const float x, const float y) const {
            return bounds().containsPoint(x, y);
        }

        void LayoutCell::updateLayout(const float maxUpScale,
                            const float minWidth, const float maxWidth,
                            const float minHeight, const float maxHeight) {
            doLayout(maxUpScale, minWidth, maxWidth, minHeight, maxHeight);
        }

        void LayoutCell::doLayout(const float maxUpScale,
                                const float minWidth, const float maxWidth,
                                const float minHeight, const float maxHeight) {
            assert(0.0f < minWidth);
            assert(0.0f < minHeight);
            assert(minWidth <= maxWidth);
            assert(minHeight <= maxHeight);

            m_scale = std::min(std::min(maxWidth / m_itemWidth, maxHeight / m_itemHeight), maxUpScale);
            const float scaledItemWidth = m_scale * m_itemWidth;
            const float scaledItemHeight = m_scale * m_itemHeight;
            const float clippedTitleWidth = std::min(m_titleWidth, maxWidth);
            const float cellWidth = std::max(minWidth, std::max(scaledItemWidth, clippedTitleWidth));
            const float cellHeight = std::max(minHeight, std::max(minHeight, scaledItemHeight) + m_titleHeight + m_titleMargin);
            const float itemY = m_y + std::max(0.0f, cellHeight - m_titleHeight - scaledItemHeight - m_titleMargin);

            m_cellBounds = LayoutBounds{m_x,
                                        m_y,
                                        cellWidth,
                                        cellHeight};
            m_itemBounds = LayoutBounds{m_x + (m_cellBounds.width - scaledItemWidth) / 2.0f,
                                        itemY,
                                        scaledItemWidth,
                                        scaledItemHeight};
            m_titleBounds = LayoutBounds{m_x + (m_cellBounds.width - clippedTitleWidth) / 2.0f,
                                            m_itemBounds.bottom() + m_titleMargin,
                                            clippedTitleWidth,
                                            m_titleHeight};
        }

        LayoutRow::LayoutRow(const float x, const float y,
                    const float cellMargin,
                    const float titleMargin,
                    const float maxWidth,
                    const size_t maxCells,
                    const float maxUpScale,
                    const float minCellWidth, const float maxCellWidth,
                    const float minCellHeight, const float maxCellHeight) :
        m_cellMargin{cellMargin},
        m_titleMargin{titleMargin},
        m_maxWidth{maxWidth},
        m_maxCells{maxCells},
        m_maxUpScale{maxUpScale},
        m_minCellWidth{minCellWidth},
        m_maxCellWidth{maxCellWidth},
        m_minCellHeight{minCellHeight},
        m_maxCellHeight{maxCellHeight},
        m_bounds{x, y, 0.0f, 0.0f} {}

        const LayoutBounds& LayoutRow::bounds() const {
            return m_bounds;
        }

        const std::vector<LayoutCell>& LayoutRow::cells() const {
            return m_cells;
        }

        const LayoutCell* LayoutRow::cellAt(const float x, const float y) const {
            for (size_t i = 0; i < m_cells.size(); ++i) {
                const LayoutCell& cell = m_cells[i];
                const LayoutBounds& cellBounds = cell.cellBounds();
                if (x > cellBounds.right()) {
                    continue;
                } else if (x < cellBounds.left()) {
                    break;
                }
                if (cell.hitTest(x, y)) {
                    return &cell;
                }
            }
            return nullptr;
        }

        bool LayoutRow::intersectsY(const float y, const float height) const {
            return m_bounds.intersectsY(y, height);
        }

        bool LayoutRow::canAddItem(const float itemWidth, const float itemHeight,
                                   const float titleWidth, const float titleHeight) const {

            float x = m_bounds.right();
            float width = m_bounds.width;
            if (!m_cells.empty()) {
                x += m_cellMargin;
                width += m_cellMargin;
            }

            auto cell = LayoutCell{x, m_bounds.top(), itemWidth, itemHeight, titleWidth, titleHeight, m_titleMargin, m_maxUpScale, m_minCellWidth, m_maxCellWidth, m_minCellHeight, m_maxCellHeight};
            width += cell.cellBounds().width;

            if (m_maxCells == 0 && width > m_maxWidth && !m_cells.empty()) {
                return false;
            }
            if (m_maxCells > 0 && m_cells.size() >= m_maxCells - 1) {
                return false;
            }

            return true;
        }

        void LayoutRow::addItem(std::any item,
                        const float itemWidth, const float itemHeight,
                        const float titleWidth, const float titleHeight) {
            float x = m_bounds.right();
            float width = m_bounds.width;
            if (!m_cells.empty()) {
                x += m_cellMargin;
                width += m_cellMargin;
            }

            auto cell = LayoutCell{x, m_bounds.top(), itemWidth, itemHeight, titleWidth, titleHeight, m_titleMargin, m_maxUpScale, m_minCellWidth, m_maxCellWidth, m_minCellHeight, m_maxCellHeight};
            width += cell.cellBounds().width;

            const float newItemRowHeight = cell.cellBounds().height - cell.titleBounds().height - m_titleMargin;
            bool readjust = newItemRowHeight > m_minCellHeight;
            if (readjust) {
                m_minCellHeight = newItemRowHeight;
                assert(m_minCellHeight <= m_maxCellHeight);
                readjustItems();
            }

            m_bounds = LayoutBounds{m_bounds.left(), m_bounds.top(), width, std::max(m_bounds.height, cell.cellBounds().height)};

            cell.setItem(std::move(item));
            m_cells.push_back(std::move(cell));
        }

        void LayoutRow::readjustItems() {
            for (auto& cell : m_cells) {
                cell.updateLayout(m_maxUpScale, m_minCellWidth, m_maxCellWidth, m_minCellHeight, m_maxCellHeight);
            }
        }

        LayoutGroup::LayoutGroup(std::string item,
                    const float x, const float y,
                    const float cellMargin, const float titleMargin, const float rowMargin,
                    const float titleHeight,
                    const float width,
                    const size_t maxCellsPerRow,
                    const float maxUpScale,
                    const float minCellWidth, const float maxCellWidth,
                    const float minCellHeight, const float maxCellHeight) :
        m_item{std::move(item)},
        m_cellMargin{cellMargin},
        m_titleMargin{titleMargin},
        m_rowMargin{rowMargin},
        m_maxCellsPerRow{maxCellsPerRow},
        m_maxUpScale{maxUpScale},
        m_minCellWidth{minCellWidth},
        m_maxCellWidth{maxCellWidth},
        m_minCellHeight{minCellHeight},
        m_maxCellHeight{maxCellHeight},
        m_titleBounds{0.0f, y, width + 2.0f * x, titleHeight},
        m_contentBounds{x, y + titleHeight + m_rowMargin, width, 0.0f},
        m_rows{} {}

        LayoutGroup::LayoutGroup(const float x, const float y,
                    const float cellMargin, const float titleMargin, const float rowMargin,
                    const float width,
                    const size_t maxCellsPerRow,
                    const float maxUpScale,
                    const float minCellWidth, const float maxCellWidth,
                    const float minCellHeight, const float maxCellHeight) :
        m_cellMargin{cellMargin},
        m_titleMargin{titleMargin},
        m_rowMargin{rowMargin},
        m_maxCellsPerRow{maxCellsPerRow},
        m_maxUpScale{maxUpScale},
        m_minCellWidth{minCellWidth},
        m_maxCellWidth{maxCellWidth},
        m_minCellHeight{minCellHeight},
        m_maxCellHeight{maxCellHeight},
        m_titleBounds{x, y, width, 0.0f},
        m_contentBounds{x, y, width, 0.0f},
        m_rows{} {}

        const std::string& LayoutGroup::item() const {
            return m_item;
        }

        const LayoutBounds& LayoutGroup::titleBounds() const {
            return m_titleBounds;
        }

        LayoutBounds LayoutGroup::titleBoundsForVisibleRect(const float y, const float height, const float groupMargin) const {
            if (intersectsY(y, height) && m_titleBounds.top() < y) {
                if (y > m_contentBounds.bottom() - m_titleBounds.height + groupMargin) {
                    return LayoutBounds{m_titleBounds.left(), m_contentBounds.bottom() - m_titleBounds.height + groupMargin, m_titleBounds.width, m_titleBounds.height};
                }
                return LayoutBounds{m_titleBounds.left(), y, m_titleBounds.width, m_titleBounds.height};
            }
            return m_titleBounds;
        }

        const LayoutBounds& LayoutGroup::contentBounds() const {
            return m_contentBounds;
        }

        LayoutBounds LayoutGroup::bounds() const {
            return LayoutBounds{m_titleBounds.left(), m_titleBounds.top(), m_titleBounds.width, m_contentBounds.bottom() - m_titleBounds.top()};
        }

        const std::vector<LayoutRow>& LayoutGroup::rows() const {
            return m_rows;
        }

        size_t LayoutGroup::indexOfRowAt(const float y) const {
            for (size_t i = 0; i < m_rows.size(); ++i) {
                const LayoutRow& row = m_rows[i];
                const LayoutBounds& rowBounds = row.bounds();
                if (y < rowBounds.bottom()) {
                    return i;
                }
            }

            return m_rows.size();
        }

        const LayoutCell* LayoutGroup::cellAt(const float x, const float y) const {
            for (size_t i = 0; i < m_rows.size(); ++i) {
                const LayoutRow& row = m_rows[i];
                const LayoutBounds& rowBounds = row.bounds();
                if (y > rowBounds.bottom()) {
                    continue;
                } else if (y < rowBounds.top()) {
                    break;
                }
                if (const LayoutCell* cell = row.cellAt(x, y)) {
                    return cell;
                }
            }

            return nullptr;
        }

        bool LayoutGroup::hitTest(const float x, const float y) const {
            return bounds().containsPoint(x, y);
        }

        bool LayoutGroup::intersectsY(const float y, const float height) const {
            return bounds().intersectsY(y, height);
        }

        void LayoutGroup::addItem(std::any item,
                        const float itemWidth, const float itemHeight,
                        const float titleWidth, const float titleHeight) {
            if (m_rows.empty()) {
                const float y = m_contentBounds.top();
                m_rows.emplace_back(m_contentBounds.left(), y, m_cellMargin, m_titleMargin, m_contentBounds.width, m_maxCellsPerRow, m_maxUpScale, m_minCellWidth, m_maxCellWidth, m_minCellHeight, m_maxCellHeight);
            }

            if (!m_rows.back().canAddItem(itemWidth, itemHeight, titleWidth, titleHeight)) {
                const LayoutBounds oldBounds = m_rows.back().bounds();
                const float y = oldBounds.bottom() + m_rowMargin;
                m_rows.emplace_back(m_contentBounds.left(), y, m_cellMargin, m_titleMargin, m_contentBounds.width, m_maxCellsPerRow, m_maxUpScale, m_minCellWidth, m_maxCellWidth, m_minCellHeight, m_maxCellHeight);

                const float newRowHeight = m_rows.back().bounds().height;
                m_contentBounds = LayoutBounds{m_contentBounds.left(), m_contentBounds.top(), m_contentBounds.width, m_contentBounds.height + newRowHeight + m_rowMargin};
            }

            const float oldRowHeight = m_rows.back().bounds().height;

            assert(m_rows.back().canAddItem(itemWidth, itemHeight, titleWidth, titleHeight));
            m_rows.back().addItem(std::move(item), itemWidth, itemHeight, titleWidth, titleHeight);

            const float newRowHeight = m_rows.back().bounds().height;
            m_contentBounds = LayoutBounds{m_contentBounds.left(), m_contentBounds.top(), m_contentBounds.width, m_contentBounds.height + (newRowHeight - oldRowHeight)};

        }

        CellLayout::CellLayout(const size_t maxCellsPerRow) :
        m_width{1.0f},
        m_cellMargin{0.0f},
        m_titleMargin{0.0f},
        m_rowMargin{0.0f},
        m_groupMargin{0.0f},
        m_outerMargin{0.0f},
        m_maxCellsPerRow{maxCellsPerRow},
        m_maxUpScale{1.0f},
        m_minCellWidth{100.0f},
        m_maxCellWidth{100.0f},
        m_minCellHeight{100.0f},
        m_maxCellHeight{100.0f},
        m_groups{},
        m_valid{false},
        m_height{0.0f} {
            invalidate();
        }

        float CellLayout::titleMargin() const {
            return m_titleMargin;
        }

        void CellLayout::setTitleMargin(const float titleMargin) {
            if (m_titleMargin != titleMargin) {
                m_titleMargin = titleMargin;
                invalidate();
            }
        }

        float CellLayout::cellMargin() const {
            return m_cellMargin;
        }

        void CellLayout::setCellMargin(const float cellMargin) {
            if (m_cellMargin != cellMargin) {
                m_cellMargin = cellMargin;
                invalidate();
            }
        }

        float CellLayout::rowMargin() const {
            return m_rowMargin;
        }

        void CellLayout::setRowMargin(const float rowMargin) {
            if (m_rowMargin != rowMargin) {
                m_rowMargin = rowMargin;
                invalidate();
            }
        }

        float CellLayout::groupMargin() const {
            return m_groupMargin;
        }

        void CellLayout::setGroupMargin(const float groupMargin) {
            if (m_groupMargin != groupMargin) {
                m_groupMargin = groupMargin;
                invalidate();
            }
        }

        float CellLayout::outerMargin() const {
            return m_outerMargin;
        }

        void CellLayout::setOuterMargin(const float outerMargin) {
            if (m_outerMargin != outerMargin) {
                m_outerMargin = outerMargin;
                invalidate();
            }
        }

        float CellLayout::minCellWidth() const {
            return m_minCellWidth;
        }

        float CellLayout::maxCellWidth() const {
            return m_maxCellWidth;
        }

        void CellLayout::setCellWidth(const float minCellWidth, const float maxCellWidth) {
            assert(0.0f < minCellWidth);
            assert(minCellWidth <= maxCellWidth);

            if (m_minCellWidth != minCellWidth || m_maxCellWidth != maxCellWidth) {
                m_minCellWidth = minCellWidth;
                m_maxCellWidth = maxCellWidth;
                invalidate();
            }
        }

        float CellLayout::minCellHeight() const {
            return m_minCellHeight;
        }

        float CellLayout::maxCellHeight() const {
            return m_maxCellHeight;
        }

        void CellLayout::setCellHeight(const float minCellHeight, const float maxCellHeight) {
            assert(0.0f < minCellHeight);
            assert(minCellHeight <= maxCellHeight);

            if (m_minCellHeight != minCellHeight || m_maxCellHeight != maxCellHeight) {
                m_minCellHeight = minCellHeight;
                m_maxCellHeight = maxCellHeight;
                invalidate();
            }
        }

        float CellLayout::maxUpScale() const {
            return m_maxUpScale;
        }

        void CellLayout::setMaxUpScale(const float maxUpScale) {
            if (m_maxUpScale != maxUpScale) {
                m_maxUpScale = maxUpScale;
                invalidate();
            }
        }

        float CellLayout::width() const {
            return m_width;
        }

        float CellLayout::height() {
            if (!m_valid) {
                validate();
            }
            return m_height;
        }

        LayoutBounds CellLayout::titleBoundsForVisibleRect(const LayoutGroup& group, const float y, const float height) const {
            return group.titleBoundsForVisibleRect(y, height, m_groupMargin);
        }

        float CellLayout::rowPosition(const float y, const int offset) {
            if (!m_valid) {
                validate();
            }

            size_t groupIndex = m_groups.size();
            for (size_t i = 0; i < m_groups.size(); ++i) {
                const LayoutGroup& candidate = m_groups[i];
                const LayoutBounds groupBounds = candidate.bounds();
                if (y + m_rowMargin > groupBounds.bottom()) {
                    continue;
                }
                groupIndex = i;
                break;
            }

            if (groupIndex == m_groups.size()) {
                return y;
            }

            if (offset == 0) {
                return y;
            }

            size_t rowIndex = m_groups[groupIndex].indexOfRowAt(y);
            int newIndex = static_cast<int>(rowIndex) + offset;
            if (newIndex < 0) {
                while (newIndex < 0 && groupIndex > 0) {
                    newIndex += static_cast<int>(m_groups[--groupIndex].rows().size());
                }
            } else if (newIndex >= static_cast<int>(m_groups[groupIndex].rows().size())) {
                while (groupIndex < m_groups.size() - 1 && newIndex >= static_cast<int>(m_groups[groupIndex].rows().size())) {
                    newIndex -= static_cast<int>(m_groups[groupIndex++].rows().size());
                }
            }

            if (groupIndex < m_groups.size()) {
                if (newIndex >= 0) {
                    rowIndex = static_cast<size_t>(newIndex);
                    if (rowIndex < m_groups[groupIndex].rows().size()) {
                        return m_groups[groupIndex].rows()[rowIndex].bounds().top();
                    }
                }
            }


            return y;
        }

        void CellLayout::invalidate() {
            m_valid = false;
        }

        void CellLayout::setWidth(const float width) {
            if (m_width != width) {
                m_width = width;
                invalidate();
            }
        }

        const std::vector<LayoutGroup>& CellLayout::groups() {
            if (!m_valid) {
                validate();
            }

            return m_groups;
        }

        const LayoutCell* CellLayout::cellAt(const float x, const float y) {
            if (!m_valid) {
                validate();
            }

            for (size_t i = 0; i < m_groups.size(); ++i) {
                const LayoutGroup& group = m_groups[i];
                const LayoutBounds groupBounds = group.bounds();
                if (y > groupBounds.bottom()) {
                    continue;
                } else if (y < groupBounds.top()) {
                    break;
                }
                if (const LayoutCell* cell = group.cellAt(x, y)) {
                    return cell;
                }
            }

            return nullptr;
        }

        void CellLayout::addGroup(std::string groupItem, const float titleHeight) {
            if (!m_valid) {
                validate();
            }

            float y = 0.0f;
            if (!m_groups.empty()) {
                y = m_groups.back().bounds().bottom() + m_groupMargin;
                m_height += m_groupMargin;
            }

            m_groups.emplace_back(std::move(groupItem), m_outerMargin, y, m_cellMargin, m_titleMargin, m_rowMargin, titleHeight, m_width - 2.0f * m_outerMargin, m_maxCellsPerRow, m_maxUpScale, m_minCellWidth, m_maxCellWidth, m_minCellHeight, m_maxCellHeight);
            m_height += m_groups.back().bounds().height;
        }

        void CellLayout::addItem(std::any item,
                        const float itemWidth, const float itemHeight,
                        const float titleWidth, const float titleHeight) {
            if (!m_valid) {
                validate();
            }

            if (m_groups.empty()) {
                m_groups.emplace_back(m_outerMargin, m_outerMargin, m_cellMargin, m_titleMargin, m_rowMargin, m_width - 2.0f * m_outerMargin, m_maxCellsPerRow, m_maxUpScale, m_minCellWidth, m_maxCellWidth, m_minCellHeight, m_maxCellHeight);
                m_height += titleHeight;
                if (titleHeight > 0.0f) {
                    m_height += m_rowMargin;
                }
            }

            const float oldGroupHeight = m_groups.back().bounds().height;
            m_groups.back().addItem(std::move(item), itemWidth, itemHeight, titleWidth, titleHeight);
            const float newGroupHeight = m_groups.back().bounds().height;

            m_height += (newGroupHeight - oldGroupHeight);
        }

        void CellLayout::clear() {
            m_groups.clear();
            invalidate();
        }

        void CellLayout::validate() {
            if (m_width <= 0.0f) {
                return;
            }

            m_height = 2.0f * m_outerMargin;
            m_valid = true;
            if (!m_groups.empty()) {
                auto copy = m_groups;
                m_groups.clear();

                for (LayoutGroup& group : copy) {
                    addGroup(group.item(), group.titleBounds().height);
                    for (const LayoutRow& row : group.rows()) {
                        for (const LayoutCell& cell : row.cells()) {
                            const LayoutBounds& itemBounds = cell.itemBounds();
                            const LayoutBounds& titleBounds = cell.titleBounds();
                            float scale = cell.scale();
                            float itemWidth = itemBounds.width / scale;
                            float itemHeight = itemBounds.height / scale;
                            addItem(std::move(cell.item()), itemWidth, itemHeight, titleBounds.width, titleBounds.height);
                        }
                    }
                }
            }
        }
    }
}
