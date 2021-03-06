Palette palette;
Filter filter;
Interface interface;

const uint8_t Palette::gammaRamp[32] = {
    0x00, 0x01, 0x03, 0x06, 0x0a, 0x0f, 0x15, 0x1c,
    0x24, 0x2d, 0x37, 0x42, 0x4e, 0x5b, 0x69, 0x78,
    0x88, 0x90, 0x98, 0xa0, 0xa8, 0xb0, 0xb8, 0xc0,
    0xc8, 0xd0, 0xd8, 0xe0, 0xe8, 0xf0, 0xf8, 0xff,
};

uint8_t Palette::contrastAdjust(uint8_t input)
{
    signed contrast = config.video.contrast - 100;
    signed result = input - contrast + (2 * contrast * input + 127) / 255;
    return max(0, min(255, result));
}

uint8_t Palette::brightnessAdjust(uint8_t input)
{
    signed brightness = config.video.brightness - 100;
    signed result = input + brightness;
    return max(0, min(255, result));
}

uint8_t Palette::gammaAdjust(uint8_t input)
{
    signed result = (signed)(pow(((double)input / 255.0),
        (double)config.video.gamma / 100.0) * 255.0 + 0.5);
    return max(0, min(255, result));
}

void Palette::update(void)
{
    for (unsigned i = 0; i < 32768; i++) {
        unsigned r = (i >> 10) & 31;
        unsigned g = (i >>  5) & 31;
        unsigned b = (i >>  0) & 31;

        r = (r << 3) | (r >> 2);
        g = (g << 3) | (g >> 2);
        b = (b << 3) | (b >> 2);

        if (config.video.useGammaRamp) {
            r = gammaRamp[r >> 3];
            g = gammaRamp[g >> 3];
            b = gammaRamp[b >> 3];
        }

        if (config.video.contrast != 100) {
            r = contrastAdjust(r);
            g = contrastAdjust(g);
            b = contrastAdjust(b);
        }

        if (config.video.brightness != 100) {
            r = brightnessAdjust(r);
            g = brightnessAdjust(g);
            b = brightnessAdjust(b);
        }

        if (config.video.gamma != 100) {
            r = gammaAdjust(r);
            g = gammaAdjust(g);
            b = gammaAdjust(b);
        }

        color[i] = (r << 16) | (g << 8) | (b << 0);
    }
}

void Filter::size(unsigned &width, unsigned &height)
{
    /* XXX: returning a value from a void function? */
    if (opened() && dl_size) {
        return dl_size(width, height);
    }
}

void Filter::render(uint32_t *output, unsigned outpitch,
    const uint16_t *input, unsigned pitch, unsigned width, unsigned height)
{
    if (opened() && dl_render) {
        return dl_render(palette.color, output, outpitch, input, pitch,
            width, height);
    }

    /*
    * This, from what I can tell, is where the magic actually happens.  The
    * pixels get rendered in the PPU, propogate through SNES::Video, and
    * refresh is called.  The refresh function is defined in the Interface
    * class where it calls this filter function.  From this filter function,
    * the pixels are sent to the rendering engine and then put on screen.
    *
    * I think.
    *
    * This for loop takes the 16-bit pixels and copies them into a 32-bit
    * pixel buffer.
    */
    for (unsigned y = 0; y < height; y++) {
        uint32_t *outputLine = output + y * (outpitch >> 2);
        const uint16_t *inputLine = input + y * (pitch >> 1);
        for (unsigned x = 0; x < width; x++) {
            *outputLine++ = palette.color[*inputLine++];
        }
    }
}

void Interface::video_refresh(const uint16_t *data, unsigned width,
    unsigned height)
{
    bool interlace = false;
    if (height >= 240) {
        interlace = true;
    }

    bool overscan = (height == 239 || height == 478);
    unsigned inpitch = interlace ? 1024 : 2048;

    uint32_t *buffer;
    unsigned outpitch;

    if (config.video.region == 0 && (height == 239 || height == 478)) {
        //NTSC overscan compensation (center image, remove 15 lines)
        data += 7 * 1024;

        if (height == 239) {
            height = 224;
        }

        if (height == 478) {
            height = 448;
        }
    }

    if (config.video.region == 1 && (height == 224 || height == 448)) {
        //PAL underscan compensation (center image, add 15 lines)
        data -= 7 * 1024;

        if (height == 224) {
            height = 239;
        }

        if (height == 448) {
            height = 478;
        }
    }

    unsigned outwidth = width;
    unsigned outheight = height;
    filter.size(outwidth, outheight);

    if (video.lock(buffer, outpitch, outwidth, outheight)) {
        filter.render(buffer, outpitch, data, inpitch, width, height);
        video.unlock();
        video.refresh();
    }

    static signed frameCounter = 0;
    static time_t previous, current;
    frameCounter++;

    time(&current);
    if (current != previous) {
        utility.setStatus({ "FPS: ", frameCounter });
        frameCounter = 0;
        previous = current;
    }
}

void Interface::audio_sample(uint16_t left, uint16_t right)
{
    if (config.audio.mute) {
        left = right = 0;
    }

    audio.sample(left, right);
}

void Interface::input_poll() { }

int16_t Interface::input_poll(bool port, SNES::Input::Device device,
    unsigned index, unsigned id)
{
    if (config.settings.focusPolicy == 1 && mainWindow.focused() == false) {
        return 0;
    }

    return inputMapper.poll(port, device, index, id);
}

void Interface::message(const string &text)
{
    MessageWindow::information(mainWindow, text);
}
