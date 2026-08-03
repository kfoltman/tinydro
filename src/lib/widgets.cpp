#include "buzzer.h"
#include "gfx.h"
#include "widgets.h"

// TODO:
// - vertical scrolling in containers
// - span management

Widget::Widget(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height, uint16_t _flags)
: pos_x{_pos_x}
, pos_y{_pos_y}
, width{_width}
, height{_height}
, flags{_flags}
, z_index{}
, next{}
, parent{}
{
}

bool Widget::onTouch(int x, int y)
{
    if (action)
    {
        click();
        action();
        return true;
    }
    return false;
}

void Widget::click()
{
    dirty();
    buzzer.click();
}

void Widget::hide()
{
    flags |= WF_HIDDEN;
    dirty();
    if (parent)
        parent->updateLayout();
}

void Widget::show()
{
    flags &= ~WF_HIDDEN;
    dirty();
    if (parent)
        parent->updateLayout();
}

void Widget::drawFrame(int line, int &sx, int &ex, uint16_t frame1, uint16_t frame2, int thickness, int padding, int &istart, int &iwidth, uint16_t bg)
{
    if (line < thickness)
    {
        renderHorizLine(pos_x + sx, pos_x + ex - line, frame1);
        renderHorizLine(pos_x + ex - line, pos_x + ex, frame2);
        ex = sx;
        return;
    }
    if (line >= height - thickness)
    {
        renderHorizLine(pos_x + sx, pos_x + sx + (height - 1 - line), frame1);
        renderHorizLine(pos_x + sx + (height - 1 - line), pos_x + ex, frame2);
        ex = sx;
        return;
    }    
    if (sx == 0 && ex > 0) {
        renderHorizLineClipped(pos_x, 0, thickness, sx, ex, frame1);
        renderHorizLineClipped(pos_x, thickness, thickness + padding, sx, ex, bg);
        sx = std::max(sx, thickness + padding);
    }
    if (sx < width && ex == width) {
        renderHorizLineClipped(pos_x, width - thickness - padding, width - thickness, sx, ex, bg);
        renderHorizLineClipped(pos_x, width - thickness, width, sx, ex, frame2);
        ex = std::min(ex, width - thickness - padding);
    }
    istart += thickness + padding;
    iwidth -= 2 * (thickness + padding);
}

Label::Label(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height, uint16_t _flags, const TextGlyphs &_text)
: Widget{_pos_x, _pos_y, _width, _height, _flags}
, text{_text}
{
}

Label::Label(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height, uint16_t _flags, const std::string &_text)
: Widget{_pos_x, _pos_y, _width, _height, _flags}
, text{_text, &font_medium}
{
}

void Label::paint(int line, int sx, int ex)
{
    int istart = 0, iwidth = width;
    if (flags & WF_BORDER)
    {
        drawFrame(line, sx, ex, 0, 0, 1, (flags & WF_INVERSE) ? 0 : 1, istart, iwidth, bg());
        if (sx >= ex)
            return;
    }
    int hpos = 0;
    if (flags & WF_ALIGN_RIGHT)
        hpos = iwidth - text.width() - istart;
    if (flags & WF_ALIGN_CENTRE)
        hpos = iwidth / 2 - text.width() / 2;
    int fontLine = line - (height / 2 - text.height() / 2);
    if (fontLine >= 0 && fontLine < text.height())
        renderTextSpan(fontLine, pos_x + sx, -hpos + sx, pos_x + ex, text, fg(), bg());
    else
        renderHorizLine(pos_x + sx, pos_x + ex, bg());
        
}

void ScrollLabel::paint(int line, int sx, int ex)
{
    int istart = 0, iwidth = width;
    drawFrame(line, sx, ex, 0, 0, 2, 2, istart, iwidth, bg());
    renderTextSpan(line, pos_x + sx, -iwidth + (globalTime % (text.width() + iwidth)) + sx, pos_x + ex, text, fg(), bg());
}

uint16_t bigradient(int line, int height, int gs1, int gs2)
{
    int halfheight = height >> 1;
    if (line < halfheight)
        return grayscale(gs1 + (gs2 - gs1) * line / halfheight);
    else
        return grayscale(gs1 + (gs2 - gs1) * (height - line - 1) / halfheight);
}

void Button::paint(int line, int sx, int ex)
{
    text.setFont(&font());
    bool on = (state & 1) != 0;
    uint16_t frame1 = on ? rgb888(96, 96, 96) : rgb888(240, 240, 240);
    uint16_t frame2 = on ? rgb888(240, 240, 240) : rgb888(64, 64, 64);
    int shift = on ? 2 : 0;
    int thickness = on ? 5 : 3;
    int istart = 0, iwidth = width;
    uint16_t bg = (state & 1) ? bigradient(line, height, 192, 255) : bigradient(line, height, 128, 224);
    drawFrame(line, sx, ex, frame1, frame2, thickness, 0, istart, iwidth, bg);
    if (sx >= ex)
        return;
    drawText(line, sx, ex, istart, iwidth, bg, shift);
}

void Button::drawText(int line, int sx, int ex, int istart, int iwidth, uint16_t bg, int shift)
{
    uint16_t fgc = fg();
    int fontLine = line - ((height - text.height()) / 2 + shift);
    if (fontLine >= 0 && fontLine < text.height()) {
        int clip = -(iwidth - (int)text.width()) / 2 + sx;
        renderTextSpan(fontLine, pos_x + sx, clip - shift, pos_x + ex, text, fgc, bg);
    } else {
        renderHorizLine(pos_x + sx, pos_x + ex, bg);
    }
}

void Button::onTick()
{
    if (state & 1) {
        if (timer > 0)
            timer--;
        else {
            state &= ~1;
            dirty();
            if (action)
                action();
        }
    }
}

bool Button::onTouch(int x, int y)
{
    if (!pressed()) {
        click();
        state |= 1;
        timer = 5;
        dirty();
    }
    return true;
}

void ToggleButton::drawText(int line, int sx, int ex, int istart, int iwidth, uint16_t bg, int shift)
{
    bool on = checked();
    text.setFont(!on ? &font_small_bold : &font_small);
    text_on.setFont(on ? &font_small_bold : &font_small);
    uint16_t fg1 = !on ? fg() : grayscale(144);
    uint16_t fg2 = on ? fg() : grayscale(144);
    int fontLine1 = line - (height / 2 - 5 * text.height() / 4 + shift);
    int fontLine2 = line - (height / 2 + text_on.height() / 4 + shift);
    if (fontLine1 >= 0 && fontLine1 < text.height()) {
        int clip = -(iwidth - (int)text.width()) / 2 + sx;
        renderTextSpan(fontLine1, pos_x + sx, clip - shift, pos_x + ex, text, fg1, bg);
    } else if (fontLine2 >= 0 && fontLine2 < text_on.height()) {
        int clip = -(iwidth - (int)text_on.width()) / 2 + sx;
        renderTextSpan(fontLine2, pos_x + sx, clip - shift, pos_x + ex, text_on, fg2, bg);
    } else {
        renderHorizLine(pos_x + sx, pos_x + ex, bg);
    }
    
}

bool ToggleButton::onTouch(int x, int y) 
{
    if (!pressed()) {
        click();
        state |= 1;
        state ^= 2;
        dirty();
        timer = 10;
    }
    return true;
}

void VerticalMenu::paint(int line, int sx, int ex)
{
    auto &rFont = font(), &vFont = fontValue();
    line += scroll_y;
    int fh = rFont.charHeight + 1;
    int menuHeight = itemCount() * fh + 2;
    if (line == 0 || line == menuHeight - 1) {
        renderHorizLine(pos_x + sx, pos_x + ex, grayscale(160));
        return;
    }
    if (line >= menuHeight) {
        renderHorizLine(pos_x + sx, pos_x + ex, bg());
        return;
    }
    line -= 1;
    int relLine = line % fh;
    int nItem = line / fh;
    const char *text = item(nItem);
    if (text) {
        if (relLine == fh - 1) {
            renderHorizLine(pos_x + sx, pos_x + ex, grayscale(160));
        } else {
            renderHorizLine(pos_x + sx, std::min(pos_x + sx + 1, pos_x + ex), grayscale(160));
            renderHorizLine(std::max(pos_x + sx, pos_x + ex - 1), pos_x + ex, grayscale(160));
            TextGlyphs menuText(text, &rFont);
            char valueBuf[128];
            bool hasValue = value(nItem, valueBuf, sizeof(valueBuf));
            uint16_t colFg = fg(), colBg = bg(), colFgValue = fgValue();
            int clip = 0;
            if (nItem == active_item && timer > 0) {
                int tv = animTime / 2 - abs(timer - animTime / 2);
                int yv = fh / 2 - abs(relLine - fh / 2);
                clip = -tv * yv * 12 / (animTime * fh / 4);
                colBg = grayscale(192 - relLine * 63 / fh);
            }
            else {
                colBg = grayscale(224 - relLine * 63 / fh);
            }
            int boundary = pos_x + ex - 1;
            if (hasValue) {
                // Would need ascent/descent for correct alignment.
                TextGlyphs valueText(valueBuf, &vFont);
                int hDiff = std::max(0, rFont.charHeight - vFont.charHeight);
                int vClip = 0;
                if (valueText.width()) {
                    boundary -= valueText.width() + 2;
                    if (boundary < (pos_x + sx + 1)) {
                        vClip = pos_x + sx + 1 - boundary;
                        boundary += vClip;
                    }
                }
                if (relLine >= hDiff) {
                    renderTextSpan(relLine - hDiff, boundary, vClip, pos_x + ex - 1, valueText, colFgValue, colBg);
                } else {
                    renderHorizLine(boundary, pos_x + ex - 1, colBg);
                }
            }
            renderTextSpan(relLine, pos_x + sx + 1, clip, boundary, menuText, colFg, colBg);
        }
    } else {
        renderHorizLine(pos_x + sx, pos_x + ex, bg());
    }
}

void VerticalMenu::onTick()
{
    if (timer > 0) {
        timer--;
        dirty();
        if (!timer) {
            activate(active_item);
        }
    }
}

bool VerticalMenu::onTouch(int x, int y)
{
    int line = y + scroll_y - pos_y - 1;
    int fh = font().charHeight + 1;
    const char *text = item(line / fh);
    if (text) {
        TextGlyphs menuText(text, &font());
        if (x >= pos_x && x < pos_x + menuText.width())
        {
            active_item = line / fh;
            timer = animTime;
            dirty();
            drag_y = -1;
            return true;
        }
    }
    drag_y = y;
    return true;
}

void VerticalMenu::onDrag(int x, int y)
{
    if (drag_y == -1)
        return;
    int fh = font().charHeight + 1;
    int menuHeight = itemCount() * fh + 2;
    scroll_y -= (y - drag_y);
    scroll_y = std::max(0, std::min(scroll_y, std::max(0, menuHeight - height)));
    drag_y = y;
    dirty();
}

WidgetContainer::WidgetContainer()
: Widget{0, 0, LINE_WIDTH, SCREEN_HEIGHT}
, first_child{}
, drag_x{}
, drag_y{}
{
    z_index = -1;
}

void WidgetContainer::setBounds(int16_t x_limit, int16_t y_limit)
{
    Widget **p = &first_child;
    if (!*p)
        return;
    int16_t sx = (*p)->pos_x, sy = (*p)->pos_y;
    int16_t ex = (*p)->pos_x + (*p)->width, ey = (*p)->pos_y + (*p)->height;
    p = &(*p)->next;
    while(*p) {
        Widget *w = *p;
        sx = std::min<int16_t>(sx, w->pos_x);
        sy = std::min<int16_t>(sy, w->pos_y);
        ex = std::max<int16_t>(ex, w->pos_x + w->width);
        ey = std::max<int16_t>(ey, w->pos_y + w->height);
        p = &(*p)->next;
    }
    pos_x = sx;
    pos_y = sy;
    width = std::min<int16_t>(x_limit, ex - sx);
    height = std::min<int16_t>(y_limit, ey - sy);
}

void WidgetContainer::add(Widget *_child)
{
    Widget **p = &first_child;
    while(*p)
        p = &(*p)->next;
    *p = _child;
    _child->parent = this;
    updateLayout();
}

bool WidgetContainer::onTouch(int x, int y)
{
    if (flags & (WF_PANX | WF_PANY)) {
        drag_x = x;
        drag_y = y;
        return true;
    }
    return false;
}

void WidgetContainer::forEach(const std::function<bool(Widget *)> &visitor)
{
    if (!visitor(this))
        return;
    for (Widget *child = first_child; child; child = child->next) {
        if (child->isContainer()) {
            WidgetContainer *container = static_cast<WidgetContainer *>(child);
            container->forEach(visitor);
        } else {
            visitor(child);
        }
    }
}

void WidgetContainer::onTick()
{
    for (Widget *child = first_child; child; child = child->next) {
        child->onTick();
    }
}

void WidgetContainer::scroll(int dx, int dy)
{
    if (dx || dy) {
        forEach([this, dx, dy](Widget *widget)->bool {
            if (widget != this) { 
                widget->pos_x += dx;
                widget->pos_y += dy;
            }
            widget->dirty();
            return true;
        });
    }
}

void WidgetContainer::onDrag(int x, int y)
{
    scroll(flags & WF_PANX ? x - drag_x : 0, flags & WF_PANY ? y - drag_y : 0);
    drag_x = x;
    drag_y = y;
}

void WidgetContainer::paint(int line, int sx, int ex)
{
    int istart = 0, iwidth = width;
    if (flags & WF_BORDER) {
        drawFrame(line, sx, ex, fg(), fg(), 2, 2, istart, iwidth, bg());
    }
    renderHorizLine(pos_x + sx, pos_x + ex, bg());
}

Screen::Screen()
{
    width = LINE_WIDTH;
    height = SCREEN_HEIGHT;
    press_widget = nullptr;
}

void Screen::onEvent(const TouchscreenEvent *event)
{
    if (event->type == TouchscreenEvent::PRESS)
    {
        press_widget = rcache.getWidgetAt(event->x, event->y);
        while(press_widget && !press_widget->onTouch(event->x, event->y))
            press_widget = press_widget->parent;
    }
    else if (event->type == TouchscreenEvent::RELEASE) {
        if (press_widget) {
            press_widget->onRelease();
        }
    }
    else if (event->type == TouchscreenEvent::ON) {
        if (press_widget) {
            press_widget->onDrag(event->x, event->y);
        }
    }
}
