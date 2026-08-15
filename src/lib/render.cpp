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
        w->next_for_render = all_widgets;
        all_widgets = w;
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
    if (num_active > 1) {
        std::sort(&active[0], &active[num_active], [](const Widget *w1, const Widget *w2) {
            return w1->z_index < w2->z_index ||
                (w1->z_index == w2->z_index && w1->clip_ex < w2->clip_ex);
        });
    }
    int16_t x_last = xs;
    while(num_active && x_last < xe) {
        Widget *next = active[num_active - 1];
        int16_t x_prev = std::max<int16_t>(next->clip_sx, x_last);
        if (x_prev > x_last)
            addSpan(vs, x_prev, nullptr);
        int16_t x_next = std::min<int16_t>(next->clip_ex, xe);
        Widget *drawn = next;
        if (x_next > x_last) {
            addSpan(vs, x_next, drawn);
            x_last = x_next;
        }
        while(num_active && active[num_active - 1]->clip_ex <= x_next)
            num_active--;
    }
    if (x_last < xe) {
        addSpan(vs, xe, nullptr);
    }
}

void RenderCache::prepareSpans(VerticalSection &vs)
{
    int num_widgets = 0;
    uint16_t vswn = vs.vs_widget;
    while(vswn != 0xFFFF && num_widgets < MAX_WIDGETS_PER_LINE) {
        prep_widgets[num_widgets++] = vs_widgets[vswn].widget;
        vswn = vs_widgets[vswn].next;
    }
    std::sort(&prep_widgets[0], &prep_widgets[num_widgets], [](const Widget *w1, const Widget *w2)->bool {
        return w1->clip_sx < w2->clip_sx;
    });

    num_active = 0;
    vs.spanStart = vs.spanEnd = num_spans;
    int x_last = 0;
    for(int i = 0; i < num_widgets; ++i) {
        Widget *w = prep_widgets[i];
        if (w->clip_sx > x_last) {
            resolve(vs, x_last, w->clip_sx);
            x_last = w->clip_sx;
        }
        active[num_active++] = w;
    }
    resolve(vs, x_last, LINE_WIDTH);
}

void RenderCache::prepare(WidgetContainer *_root)
{
    if (root == _root && !needLayout)
        return;

    root = _root;
    all_widgets = nullptr;
    prepareWidgets(root, 0, LINE_WIDTH, 0, SCREEN_HEIGHT);
    // Note: touched prevents the same coordinate being added multiple
    // times, this is O(N) but reduces the vector size for the Y coordinate
    // sorting later, which is O(NlogN); it also reduces max size needed
    // for y_coords.
    int num_y_coords = 0;
    for (Widget *widget = all_widgets; widget; widget = widget->next_for_render) {
        int16_t sy = widget->clip_sy;
        if (!touched.test(sy)) {
            y_coords[num_y_coords++] = sy;
            touched.set(sy);
        }
        int16_t ey = widget->clip_ey;
        if (!touched.test(ey)) {
            y_coords[num_y_coords++] = ey;
            touched.set(ey);
        }
    };
    std::sort(&y_coords[0], &y_coords[num_y_coords]);
    int16_t y_last = 0;
    num_y_sections = 0;
    // Skip the initial 0 if needed
    int y_start = (num_y_coords && y_coords[0] == 0) ? 1 : 0;
    for (int i = y_start; i < num_y_coords; ++i) {
        int16_t y = y_coords[i];
        // Zero the set bits in the bitset for the next time - not sure
        // if it wins anything vs. bulk clearing, as this would only be a simple
        // write of ~11 dwords.
        touched.reset(y);
        y_sections[num_y_sections++] = VerticalSection{y_last, y, {}};
        y_last = y;
    }
    if (y_last < SCREEN_HEIGHT)
        y_sections[num_y_sections++] = VerticalSection{y_last, SCREEN_HEIGHT, {}};
    num_vs_widgets = 0;
    for (Widget *widget = all_widgets; widget; widget = widget->next_for_render) {
        auto i = std::lower_bound(&y_sections[0], &y_sections[num_y_sections], widget->clip_sy,
            [](const VerticalSection &vs, int16_t value) -> bool { return vs.ys < value; } );
        while (i != &y_sections[num_y_sections] && i->ys < widget->clip_ey) {
            addWidget(*i, widget);
            ++i;
        }
    };
    num_spans = 0;
    for(int i = 0; i < num_y_sections; ++i)
        prepareSpans(y_sections[i]);
    needLayout = false;
#if 0
    printf("Size %d sections %d spans %d vs_widgets %d\n", sizeof(*this), num_y_sections, num_spans, num_vs_widgets);
    fflush(stdout);
#endif
}

void RenderCache::render(Display &display)
{
    bool root_changed = !(root->flags & WF_UNCHANGED);
    for (Widget *widget = all_widgets; widget; widget = widget->next_for_render) {
        widget->prePaint();
    }
    for(int i = 0; i < num_y_sections; ++i) {
        auto &vs = y_sections[i];
        for (int line = vs.ys; line < vs.ye; ++line) {
            int x_last = 0;
            int span_start = 0;
            for(uint16_t j = vs.spanStart; j < vs.spanEnd; ++j) {
                Widget *w = spanWidget[j];
                int end = spanEnd[j];
                bool drawn = root_changed;
                if (w) {
                    drawn = !(w->flags & WF_UNCHANGED) || root_changed;
                    if (drawn) {
                        w->paint(line - w->pos_y, span_start - w->pos_x, end - w->pos_x);
                    } else {
                        if (x_last == span_start)
                            x_last = end;
                    }
                } else if (root_changed) {
                    // Blank area not covered by widgets - use fill instead of copy,
                    // but copy anything that has been drawn so far but hasn't been
                    // copied to screen.
                    if (span_start > x_last)
                        display.copy(line, x_last, span_start, line_buffer + x_last);
                    display.fill(line, span_start, end, 0xF800);
                    x_last = end;
                }
                if (!drawn) {
                    // Nothing has been drawn this time, so copy anything that has
                    // been drawn before and skip past the display area that stays the
                    // same.
                    if (span_start > x_last)
                        display.copy(line, x_last, span_start, line_buffer + x_last);
                    x_last = end;
                }
                span_start = end;
            }
            if (x_last < LINE_WIDTH)
                display.copy(line, x_last, LINE_WIDTH, line_buffer + x_last);
        }
    }
    root->flags |= WF_UNCHANGED;
    for (Widget *widget = all_widgets; widget; widget = widget->next_for_render) {
        widget->postPaint();
    }
}

Widget *RenderCache::getWidgetAt(int16_t x, int16_t y)
{
    auto i = std::lower_bound(&y_sections[0], &y_sections[num_y_sections], y,
        [](const VerticalSection &vs, int16_t value) -> bool { return vs.ye < value; } );
    if (i == &y_sections[num_y_sections])
        return nullptr;
    const VerticalSection &vs = *i;
    if (y >= vs.ys && y < vs.ye) {
        auto *j = std::lower_bound(&spanEnd[vs.spanStart], &spanEnd[vs.spanEnd], x,
            [](const uint16_t &spanEnd, int16_t value) -> bool { return spanEnd < value; } );
        if (j != &spanEnd[vs.spanEnd] && (j == &spanEnd[vs.spanStart] || x >= j[-1] ) && x < *j)
            return spanWidget[j - &spanEnd[0]];
    }
    return nullptr;
}

