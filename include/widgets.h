#pragma once

#include <bitset>
#include <cassert>
#include <functional>
#include <cmath>
#include "gfx.h"
#include "bmpfont.h"
#include "touchscreen.h"

#include "IManagedScreen.h"

#define WF_BORDER (1 << 0)
#define WF_INVERSE (1 << 1)
#define WF_ALIGN_RIGHT (1 << 4)
#define WF_ALIGN_CENTRE (1 << 5)
#define WF_PANX (1 << 6)
#define WF_PANY (1 << 7)
#define WF_GLASS (1 << 12)
#define WF_SEETHRU (1 << 13)
#define WF_HIDDEN (1 << 14)
#define WF_UNCHANGED (1 << 15)

class WidgetContainer;

class Widget
{
public:
    int16_t pos_x, pos_y;
    uint16_t width, height, flags;
    int16_t z_index;
    int16_t clip_sx, clip_ex, clip_sy, clip_ey;
    Widget *next, *next_for_render;
    WidgetContainer *parent;
    std::function<void()> action;

    Widget() : pos_x{}, pos_y{}, width{}, height{}, flags{}, z_index{}, next{nullptr}, parent{nullptr} {}
    Widget(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height, uint16_t _flags = 0);

    virtual void dirty() { flags &= ~WF_UNCHANGED; }
    virtual void prePaint() {}
    virtual void onTick() {}
    virtual void paint(int line, int sx, int ex) {}
    virtual void paintWithBackLayers(int line, int sx, int ex, Widget *widgets[], int num_widgets);
    virtual bool onTouch(int x, int y);
    virtual void onRelease() {}
    virtual void onDrag(int x, int y) {}
    virtual void click();
    virtual void postPaint() { flags |= WF_UNCHANGED; }
    virtual bool isContainer() const { return false; }
    virtual uint16_t bg() const { return (flags & WF_INVERSE) ? grayscale(0) : grayscale(224); }
    virtual uint16_t fg() const { return (flags & WF_INVERSE) ? grayscale(224) : (flags & WF_SEETHRU ? grayscale(128) : grayscale(0)); }

    void hide();
    void show();
    bool isShown() const { return (flags & WF_HIDDEN) == 0; }

    void drawFrame(int line, int &sx, int &ex, uint16_t frame1, uint16_t frame2, int thickness, int padding, int &istart, int &iwidth, uint16_t bg);

    inline void paintBackSpan(int line, int sx, int ex, Widget *widgets[], int num_widgets, int sxSpan, int exSpan) {
        if (num_widgets) {
            Widget *prev = widgets[num_widgets - 1];
            int dx = pos_x - prev->pos_x;
            int dy = pos_y - prev->pos_y;
            sx = std::max(sx, sxSpan);
            ex = std::min(ex, exSpan);
            if (ex > sx)
                prev->paintWithBackLayers(line + dy, sx + dx, ex + dx, widgets, num_widgets - 1);
        }
    }
};

class Label: public Widget
{
public:
    TextGlyphs text;
    
    Label() : Widget{}, text{} {}
    Label(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height, uint16_t _flags, const TextGlyphs &_text);
    Label(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height, uint16_t _flags, const std::string &_text);
    virtual void paintWithBackLayers(int line, int sx, int ex, Widget *widgets[], int num_widgets);
    void setText(const std::string &_text) { if (text.setText(_text)) { dirty(); } }
    void setFont(const Font *_font) { text.setFont(_font); }
    void paint(int line, int sx, int ex) override;
    virtual uint16_t bg() const { if (flags & WF_SEETHRU) return grayscale(192); return Widget::bg(); }
};

class ScrollLabel: public Label
{
public:
    ScrollLabel() = default;
    ScrollLabel(uint16_t _pos_x, uint16_t _pos_y, uint16_t _width, uint16_t _height, uint16_t _flags, const TextGlyphs &_text)
    : Label(_pos_x, _pos_y, _width, _height, _flags, _text) {}
    ScrollLabel(uint16_t _pos_x, uint16_t _pos_y, uint16_t _width, uint16_t _height, uint16_t _flags, const std::string &_text)
    : Label(_pos_x, _pos_y, _width, _height, _flags, _text) {}
    void paint(int line, int sx, int ex) override;
    void postPaint() override { }
};

class Button: public Label
{
protected:
    uint32_t state;
public:
    int timer;
    
    Button() : Label(), state{}, timer{} {}
    Button(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height, const TextGlyphs &_text)
    : Label(_pos_x, _pos_y, _width, _height, 0, _text)
    , state(0), timer(0) {}
    Button(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height, const std::string &_text)
    : Label(_pos_x, _pos_y, _width, _height, 0, _text)
    , state(0), timer(0) {}
    inline bool pressed() const { return state & 1; }
    inline bool checked() const { return state & 2; }
    void paint(int line, int sx, int ex) override;
    virtual const Font &font() const { return (state & 1) ? font_small_bold : font_small; }
    virtual uint16_t bg() const { return 0xFFFF; }
    virtual uint16_t fg() const { return 0; }
    bool onTouch(int x, int y) override;
    void onTick() override;
    virtual void drawText(int line, int sx, int ex, int istart, int iwidth, uint16_t bg, int shift);
};

class ToggleButton: public Button
{
public:
    TextGlyphs text_on;

    ToggleButton(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height, const std::string &_text, const std::string &_text_on)
    : Button(_pos_x, _pos_y, _width, _height, _text)
    , text_on(_text_on, nullptr) {}
    virtual void drawText(int line, int sx, int ex, int istart, int iwidth, uint16_t bg, int shift);
    bool onTouch(int x, int y) override;
};

class VerticalMenu: public Widget
{
protected:
    static constexpr int animTime = 10;
public:
    int timer;
    int active_item;
    int drag_y;
    int scroll_y;

    VerticalMenu(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height)
    : Widget{_pos_x, _pos_y, _width, _height, WF_BORDER}
    , timer{0}
    , active_item{-1}
    , drag_y{-1}
    , scroll_y{0}
    {}
    virtual const char *item(int index) const = 0;
    virtual bool value(int index, char *buffer, unsigned int size) const = 0;
    virtual int itemCount() const = 0;
    virtual void activate(int index) {}

    void paint(int line, int sx, int ex) override;
    virtual const Font &font() const { return font_medium_bold; }
    virtual const Font &fontValue() const { return font_medium; }
    virtual uint16_t bg() const { return grayscale(208); }
    virtual uint16_t fg() const { return grayscale(0); }
    virtual uint16_t fgValue() const { return (flags & WF_INVERSE) ? grayscale(192) : grayscale(64); }
    bool onTouch(int x, int y) override;
    void onDrag(int x, int y) override;
    void onRelease() { drag_y = -1; }
    void onTick() override;
    void scrollToTop() { scroll_y = 0; dirty(); }
};

template<typename Owner>
struct SimpleMenuItem
{
    const char *name;
    std::function<void(Owner *)> activate;
    std::function<bool(Owner *, char *buffer, unsigned int size)> value;
};

template<typename Owner>
class SimpleMenu: public VerticalMenu
{
protected:
    const SimpleMenuItem<Owner> *items;
    int n_items;
    Owner *owner;
    bool valid(int index) const { return (index >= 0 && index < n_items); }
public:
    SimpleMenu(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height, const SimpleMenuItem<Owner> *_items, int _n_items, Owner *_owner)
    : VerticalMenu(_pos_x, _pos_y, _width, _height), items{_items}, n_items{_n_items}, owner(_owner) {}
    const char *item(int index) const override { return valid(index) ? items[index].name : nullptr; }
    virtual bool value(int index, char *buffer, unsigned int size) const { return valid(index) && items[index].value ? items[index].value(owner, buffer, size) : false; }
    int itemCount() const override { return n_items; }
    void activate(int index) override {
        if (items[index].activate) {
            items[index].activate(owner);
        }
    }
};

class WidgetContainer: public Widget
{
public:
    Widget *first_child;
    int drag_x, drag_y;
    
    WidgetContainer();
    void setBounds(int16_t x_limit, int16_t y_limit);
    void add(Widget *_child);
    bool onTouch(int x, int y) override;
    void forEach(const std::function<bool(Widget *)> &visitor);
    void scroll(int dx, int dy);
    bool isContainer() const override { return true; }
    void onDrag(int x, int y) override;
    void paint(int line, int sx, int ex) override;
    void onTick() override;
    virtual void updateLayout() { parent->updateLayout(); }
};

struct VerticalSection
{
    int16_t ys, ye;
    uint16_t spanStart{}, spanEnd{};
    uint16_t vs_widget{0xFFFF};
};

struct VSWidget
{
    Widget *widget;
    uint16_t next;
};

class RenderCache
{
protected:
    // Good generic defaults, maybe
    //enum { MAX_SPANS = 512, MAX_LAYERS = 32, MAX_VS_WIDGETS = 256, MAX_WIDGETS_PER_LINE = 128 };
    // Sufficient for now
    enum { MAX_SPANS = 128, MAX_LAYERS = 32, MAX_VS_WIDGETS = 128, MAX_WIDGETS_PER_LINE = 64 };
    static constexpr uint16_t SPAN_STACK = 0x8000;
    WidgetContainer *root;
    bool needLayout{true};

    // Persistent output of prepare() - needed for drawing and collision detection
    std::array<VerticalSection, SCREEN_HEIGHT> y_sections;
    std::array<uint16_t, MAX_SPANS> span_end;
    std::array<Widget *, MAX_SPANS> span_widget;
    uint16_t num_y_sections{0};
    uint16_t num_spans{0};

    // Temporary data structures used in the prepare method (this memory could be shared with other things
    // if I *really* run out of RAM; or could be allocated on stack if I can guarantee that the stack
    // is large enough)
    Widget *all_widgets; // intrusive linked list
    std::bitset<SCREEN_HEIGHT + 1> touched;
    std::array<int16_t, SCREEN_HEIGHT + 1> y_coords;
    std::array<Widget *, MAX_LAYERS> active;
    std::array<VSWidget, MAX_VS_WIDGETS> vs_widgets;
    std::array<Widget *, MAX_WIDGETS_PER_LINE> prep_widgets;
    uint16_t num_active{0};
    uint16_t num_vs_widgets{0};

    void prepareWidgets(Widget *w, int16_t sx, int16_t ex, int16_t sy, int16_t ey);
    void prepareSpans(VerticalSection &vs);
    void resolve(VerticalSection &vs, int16_t xs, int16_t xe);
    inline void addSpan(VerticalSection &vs, int16_t end, Widget *widget) {
        assert(end <= LINE_WIDTH);
        if (num_spans >= MAX_SPANS)
            return;
        if (vs.spanStart != vs.spanEnd && span_widget[vs.spanEnd - 1] == widget) {
            span_end[vs.spanEnd - 1] = end;
        } else {
            span_end[num_spans] = end;
            span_widget[num_spans++] = widget;
            vs.spanEnd = num_spans;
        }
    }
    inline void addStackSpan(VerticalSection &vs, int16_t start, int16_t end, int first_solid) {
        assert(end <= LINE_WIDTH);
        int count = std::min<int>(MAX_SPANS - num_spans, num_active - first_solid);
        for (int i = 0; i < count; ++i) {
            if (active[first_solid + i]->clip_ex > start) {
                span_end[num_spans] = end | (i == count - 1 ? 0 : SPAN_STACK);
                span_widget[num_spans++] = active[first_solid + i];
            }
        }
        vs.spanEnd = num_spans;
    }
    inline void addWidget(VerticalSection &vs, Widget *widget) {
        vs_widgets[num_vs_widgets] = VSWidget{widget, vs.vs_widget};
        vs.vs_widget = num_vs_widgets++;
    }
public:
    void prepare(WidgetContainer *_root);
    void render(Display &display);
    Widget *getWidgetAt(int16_t x, int16_t y);
    void dirty() { needLayout = true; }
};

class Screen: public WidgetContainer, public IManagedScreen
{
protected:
    static RenderCache shared_rcache;
    RenderCache *rcache;
    Widget *press_widget;
public:
    // ScreenManager interface / default methods
    void begin() { init(); }
    void start() {
        rcache = &shared_rcache;
        updateLayout();
    }
    bool end() {
        rcache = nullptr;
        return true;
    }
    void draw() {}
public:
    Screen();
    virtual void init() = 0;
    virtual void loop() = 0;
    virtual void onEvent(const TouchscreenEvent *event);
    virtual void onKey(int key) {}
    void render(Display &display) {
        if (rcache) {
            rcache->prepare(this);
            rcache->render(display);
        }
    }
    void updateLayout() {
        dirty();
        if (rcache)
            rcache->dirty();
    }
};

#define BEGIN_MENU(screen, name) \
    const SimpleMenuItem<screen> screen::mitems##name[] = {

#define END_MENU(screen, name) \
    }; \
    const int screen::num##name = std::size(mitems##name);

#define MENU(screen, name) \
    static const SimpleMenuItem<screen> mitems##name[]; \
    static const int num##name; \
    SimpleMenu<MainScreen> menu##name{x_right_part + 10, 2, LINE_WIDTH - x_right_part - 10, SCREEN_HEIGHT - 4, mitems##name, num##name, this};

