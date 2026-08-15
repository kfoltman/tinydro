#pragma once

#include <cassert>
#include "gfx.h"
#include "bmpfont.h"
#include "touchscreen.h"
#include <functional>

#include "IManagedScreen.h"

#define WF_BORDER (1 << 0)
#define WF_INVERSE (1 << 1)
#define WF_ALIGN_RIGHT (1 << 4)
#define WF_ALIGN_CENTRE (1 << 5)
#define WF_PANX (1 << 6)
#define WF_PANY (1 << 7)
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
    Widget *next;
    WidgetContainer *parent;
    std::function<void()> action;
    
    Widget() : pos_x{}, pos_y{}, width{}, height{}, flags{}, z_index{}, next{nullptr}, parent{nullptr} {}
    Widget(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height, uint16_t _flags = 0);
    
    virtual void dirty() { flags &= ~WF_UNCHANGED; }
    virtual void prePaint() {}
    virtual void onTick() {}
    virtual void paint(int line, int sx, int ex) {}
    virtual bool onTouch(int x, int y);
    virtual void onRelease() {}
    virtual void onDrag(int x, int y) {}
    virtual void click();
    virtual void postPaint() { flags |= WF_UNCHANGED; }
    virtual bool isContainer() const { return false; }
    virtual uint16_t bg() const { return (flags & WF_INVERSE) ? grayscale(0) : grayscale(224); }
    virtual uint16_t fg() const { return (flags & WF_INVERSE) ? grayscale(224) : grayscale(0); }

    void hide();
    void show();
    bool isShown() const { return (flags & WF_HIDDEN) == 0; }

    void drawFrame(int line, int &sx, int &ex, uint16_t frame1, uint16_t frame2, int thickness, int padding, int &istart, int &iwidth, uint16_t bg);
};

class Label: public Widget
{
public:
    TextGlyphs text;
    
    Label() : Widget{}, text{} {}
    Label(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height, uint16_t _flags, const TextGlyphs &_text);
    Label(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height, uint16_t _flags, const std::string &_text);
    void setText(const std::string &_text) { if (text.setText(_text)) { dirty(); } }
    void setFont(const Font *_font) { text.setFont(_font); }
    void paint(int line, int sx, int ex) override;
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

struct PixelSpan
{
    int16_t end;
    Widget *widget;
};

struct VerticalSection
{
    int16_t ys, ye;
    std::vector<Widget *>widgets;
    std::vector<PixelSpan> spans;
    
    inline void addSpan(int16_t end, Widget *widget) {
        assert(end <= LINE_WIDTH);
        if (!spans.empty() && spans.rbegin()->widget == widget) {
            spans.rbegin()->end = end;
        } else {
            spans.push_back({end, widget});
        }
    }
};

class RenderCache
{
protected:
    WidgetContainer *root;
    std::vector<Widget *> all_widgets;
    std::vector<int16_t> y_coords;
    std::vector<VerticalSection> y_sections;
    std::vector<Widget *> active;
    bool needLayout{true};

    void prepareWidgets(Widget *w, int16_t sx, int16_t ex, int16_t sy, int16_t ey);
    void prepareSpans(VerticalSection &vs);
    void resolve(VerticalSection &vs, int16_t xs, int16_t xe);
public:
    void prepare(WidgetContainer *_root);
    void render(Display &display);
    Widget *getWidgetAt(int16_t x, int16_t y);
    void dirty() { needLayout = true; }
};

class Screen: public WidgetContainer, public IManagedScreen
{
protected:
    RenderCache rcache;
    Widget *press_widget;
public:
    // ScreenManager interface / default methods
    void begin() { init(); }
    void start() { dirty(); }
    bool end() { return true; }
    void draw() {}
public:
    Screen();
    virtual void init() = 0;
    virtual void loop() = 0;
    virtual void onEvent(const TouchscreenEvent *event);
    void render(Display &display) { rcache.prepare(this); rcache.render(display); }
    void updateLayout() { dirty(); rcache.dirty(); }
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

