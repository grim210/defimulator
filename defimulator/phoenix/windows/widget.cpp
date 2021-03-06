void Widget::setFont(Font &font)
{
    widget->font = font.font->font;
    SendMessage(widget->window, WM_SETFONT, (WPARAM)font.font->font, 0);
}

bool Widget::visible(void)
{
    return GetWindowLong(widget->window, GWL_STYLE) & WS_VISIBLE;
}

void Widget::setVisible(bool visible)
{
    ShowWindow(widget->window, visible ? SW_SHOWNORMAL : SW_HIDE);
}

bool Widget::enabled(void)
{
    return IsWindowEnabled(widget->window);
}

void Widget::setEnabled(bool enabled)
{
    EnableWindow(widget->window, enabled);
}

bool Widget::focused(void)
{
    return (GetForegroundWindow() == widget->window);
}

void Widget::setFocused(void)
{
    if (visible() == false) {
        setVisible(true);
    }

    SetFocus(widget->window);
}

void Widget::setGeometry(unsigned x, unsigned y, unsigned width,
    unsigned height)
{
    SetWindowPos(widget->window, NULL, x, y, width, height, SWP_NOZORDER);
}

Widget::Widget(void)
{
    OS::os->objects.append(this);
    widget = new Widget::Data;
    widget->window = 0;
    widget->font = OS::os->proportionalFont;
}
