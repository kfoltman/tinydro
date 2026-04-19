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
, text{_text, &font_sans}
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

void Button::prePaint()
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

WidgetContainer::WidgetContainer()
: Widget{0, 0, LINE_WIDTH, SCREEN_HEIGHT}
, first_child{}
, drag_x{}
, drag_y{}
{
    z_index = -1;
}

void WidgetContainer::add(Widget *_child)
{
    Widget **p = &first_child;
    while(*p)
        p = &(*p)->next;
    *p = _child;
    _child->parent = this;
}

bool WidgetContainer::onTouch(int x, int y)
{
    drag_x = x;
    drag_y = y;
    return true;
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
    scroll(x - drag_x, y - drag_y);
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

void RenderCache::prepareWidgets(Widget *w, int16_t sx, int16_t ex, int16_t sy, int16_t ey)
{
    if (w->flags & WF_HIDDEN)
        return;
    sx = w->clip_sx = std::max(sx, w->pos_x);
    ex = w->clip_ex = std::min<int16_t>(ex, w->pos_x + w->width);
    sy = w->clip_sy = std::max(sy, w->pos_y);
    ey = w->clip_ey = std::min<int16_t>(ey, w->pos_y + w->height);
    if (sx < ex && sy < ey) {
        all_widgets.push_back(w);
        if (w->isContainer()) {
            int border = (w->flags & WF_BORDER) ? 2 : 0;
            sx += border;
            ex -= border;
            sy += border;
            ey -= border;
            WidgetContainer *container = (WidgetContainer *)w;
            for(Widget *c = container->first_child; c; c = c->next) {
                prepareWidgets(c, sx, ex, sy, ey);
            }
        }
    }
}

void RenderCache::resolve(VerticalSection &vs, int16_t xs, int16_t xe)
{
    if (xs >= xe)
        return;
    // Sort so that the last item is the one is the with the fewest pixels remaining to draw
    std::sort(active.begin(), active.end(), [](const Widget *w1, const Widget *w2) { return w1->clip_ex > w2->clip_ex; });
    int16_t x_last = xs;
    while(!active.empty() && x_last < xe) {
        Widget *next = *active.rbegin();
        int16_t x_next = std::min<int16_t>(next->clip_ex, xe);
        Widget *drawn = next;
        for (Widget *candidate: active) {
            if (candidate->z_index > drawn->z_index)
                drawn = candidate;
        }
        if (x_next > x_last) {
            vs.addSpan(x_last, x_next, drawn);
            x_last = x_next;
        }
        if (next->clip_ex <= x_next)
            active.pop_back();
    }
    if (x_last < xe) {
        vs.addSpan(x_last, xe, nullptr);
    }
}

void RenderCache::prepareSpans(VerticalSection &vs)
{
    std::sort(vs.widgets.begin(), vs.widgets.end(), [](const Widget *w1, const Widget *w2)->bool {
        return w1->clip_sx < w2->clip_sx;
    });

    active.clear();
    vs.spans.clear();
    int x_last = 0;
    for(Widget *w: vs.widgets) {
        if (w->clip_sx > x_last) {
            resolve(vs, x_last, w->clip_sx);
            x_last = w->clip_sx;
        }
        active.push_back(w);
    }
    resolve(vs, x_last, LINE_WIDTH);
}

void RenderCache::prepare(WidgetContainer *_root)
{
    root = _root;
    // Note: this will not resize those structs. However, it will
    // destroy the inner vectors in vertical sections.
    all_widgets.clear();
    prepareWidgets(root, 0, LINE_WIDTH, 0, SCREEN_HEIGHT);
    y_coords.clear();
    y_sections.clear();
    y_coords.reserve(all_widgets.size() * 2);
    y_sections.reserve(all_widgets.size() * 2);
    for (Widget *widget: all_widgets) {
        y_coords.push_back(std::max<int16_t>(0, widget->clip_sy));
        y_coords.push_back(std::min<int16_t>(widget->clip_ey, SCREEN_HEIGHT));
    };
    std::sort(y_coords.begin(), y_coords.end());
    // This is in sorted order but may contain duplicate coordinates.
    int16_t y_last = 0;
    for (auto y: y_coords) {
        if (y <= y_last)
            continue;
        y_sections.push_back(VerticalSection{y_last, y, {}});
        y_last = y;
    }
    if (y_last < SCREEN_HEIGHT)
        y_sections.push_back(VerticalSection{y_last, SCREEN_HEIGHT, {}});
    for (Widget *widget: all_widgets) {
        auto i = std::lower_bound(y_sections.begin(), y_sections.end(), widget->clip_sy,
            [](const VerticalSection &vs, int16_t value) -> bool { return vs.ys < value; } );
        while (i != y_sections.end() && i->ys < widget->clip_ey) {
            i->widgets.push_back(widget);
            ++i;
        }
    };
    for(auto &vs: y_sections)
        prepareSpans(vs);
}

void RenderCache::render(Display &display)
{
    bool root_changed = !(root->flags & WF_UNCHANGED);
    for(Widget *widget: all_widgets) {
        widget->prePaint();
    }
    for(auto const &vs: y_sections) {
        for (int line = vs.ys; line < vs.ye; ++line) {
            int x_last = 0;
            for(const PixelSpan &span: vs.spans) {
                Widget *w = span.widget;
                bool drawn = root_changed;
                if (w) {
                    drawn = !(w->flags & WF_UNCHANGED) || root_changed;
                    if (drawn) {
                        w->paint(line - w->pos_y, span.start - w->pos_x, span.end - w->pos_x);
                    } else {
                        if (x_last == span.start)
                            x_last = span.end;
                    }
                } else if (root_changed) {
                    if (span.start > x_last)
                        display.copy(line, x_last, span.start, line_buffer + x_last);
                    x_last = span.end;
                    display.fill(line, span.start, span.end, 0xF800);
                }
                if (!drawn) {
                    if (span.start > x_last)
                        display.copy(line, x_last, span.start, line_buffer + x_last);
                    x_last = span.end;
                }
            }
            if (x_last < LINE_WIDTH)
                display.copy(line, x_last, LINE_WIDTH, line_buffer + x_last);
        }
    }
    root->flags |= WF_UNCHANGED;
    for(Widget *widget: all_widgets) {
        widget->postPaint();
    }
}

Widget *RenderCache::getWidgetAt(int16_t x, int16_t y)
{
    auto i = std::lower_bound(y_sections.begin(), y_sections.end(), y,
        [](const VerticalSection &vs, int16_t value) -> bool { return vs.ye < value; } );
    if (i == y_sections.end())
        return nullptr;
    const VerticalSection &vs = *i;
    if (y >= vs.ys && y < vs.ye) {
        auto j = std::lower_bound(vs.spans.begin(), vs.spans.end(), x,
            [](const PixelSpan &span, int16_t value) -> bool { return span.end < value; } );
        if (j != vs.spans.end() && x >= j->start && x < j->end)
            return j->widget;
    }
    return nullptr;
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
