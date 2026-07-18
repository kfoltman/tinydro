#include "buzzer.h"
#include "gfx.h"
#include "widgets.h"

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

