#include <ui/settings.h>

InputSettings inputSettings;
static InputMapper::AbstractInput *activeInput = 0;

void InputSettings::create(void)
{
    Window::create(0, 0, 256, 256, "Input Settings");
    application.addWindow(this, "InputSettings", "160,160");

    setFont(application.proportionalFontBold);
    setStatusVisible();

    activeInput = 0;
    activeMouse = 0;

    unsigned x = 5, y = 5, listboxsize = 265;
    unsigned height = Style::ButtonHeight;

    portLabel.create(*this, x, y, 50, Style::ComboBoxHeight, "Port:");
    portBox.create(*this, x + 50, y, 200, Style::ComboBoxHeight);
    portBox.addItem(inputMapper.port1.name);
    portBox.addItem(inputMapper.port2.name);
    deviceLabel.create(*this, x + 255, y, 50, Style::ComboBoxHeight,
        "Device:");
    deviceBox.create(*this, x + 305, y, 200, Style::ComboBoxHeight);
    y += Style::ComboBoxHeight + 5;

    mappingList.create(*this, x, y, 505, listboxsize, "Name\tMapping");
    mappingList.setHeaderVisible();
    mappingList.setFocused();
    y += listboxsize + 5;

    mouseXaxis.create(*this, x, y, 100, height, "Mouse X-axis");
    mouseXaxis.setVisible(false);
    mouseYaxis.create(*this, x + 105, y, 100, height, "Mouse Y-axis");
    mouseYaxis.setVisible(false);
    mouseLeft.create(*this, x, y, 100, height, "Mouse Left");
    mouseLeft.setVisible(false);
    mouseMiddle.create(*this, x + 105, y, 100, height, "Mouse Middle");
    mouseMiddle.setVisible(false);
    mouseRight.create(*this, x + 105 + 105, y, 100, height, "Mouse Right");
    mouseRight.setVisible(false);
    clearButton.create(*this, 515 - 85, y, 80, height, "Clear");
    y += height + 5;

    setGeometry(0, 0, 515, y);

    portChanged();
    portBox.onChange = { &InputSettings::portChanged, this };
    deviceBox.onChange = { &InputSettings::deviceChanged, this };
    mappingList.onActivate = { &InputSettings::assignInput, this };

    mouseXaxis.onTick = []() {
        inputSettings.setMapping(Scancode::encode(
            mouse(inputSettings.activeMouse)[Mouse::Xaxis]));
    };

    mouseYaxis.onTick = []() {
        inputSettings.setMapping(Scancode::encode(
            mouse(inputSettings.activeMouse)[Mouse::Yaxis]));
    };

    mouseLeft.onTick = []() {
        inputSettings.setMapping(Scancode::encode(
            mouse(inputSettings.activeMouse)[Mouse::Button0]));
    };

    mouseMiddle.onTick = []() {
        inputSettings.setMapping(Scancode::encode(
            mouse(inputSettings.activeMouse)[Mouse::Button1]));
    };

    mouseRight.onTick = []() {
        inputSettings.setMapping(Scancode::encode(
            mouse(inputSettings.activeMouse)[Mouse::Button2]));
    };

    clearButton.onTick = { &InputSettings::clearInput, this };
    onClose = []() { inputSettings.endAssignment(); return true; };
}

void InputSettings::portChanged(void)
{
    deviceBox.reset();
    InputMapper::ControllerPort &port = (
      portBox.selection() == 0
        ? (InputMapper::ControllerPort&)inputMapper.port1
        : (InputMapper::ControllerPort&)inputMapper.port2
    );

    for (unsigned i = 0; i < port.size(); i++) {
        deviceBox.addItem(port[i]->name);
    }

    deviceChanged();
}

/*
* Prevents breakage is someone doesn't realize I've overloaded
* a the same function with another parameter.  This will allow
* the setVisible() function to behave identically to
* setVisible(false) for this particular class.  Which will then
* call TopLevelWindow::setVisible() as originally intended.
*/
void InputSettings::setVisible(void)
{
    InputSettings::setVisible(false);
}

void InputSettings::setVisible(bool hotkeys)
{
    /*
    * XXX: This is a pretty hacky solution to make the hotkeys UX
    * better than it was.  While you could always get to the hotkey
    * configuration (since like 2 commits ago) from the Input Settings,
    * it wasn't super obvious where to find the hotkey config.  With
    * this method it simply selects the device (hard-coded, of course)
    * that the hotkeys is configured to be and then sets the settings
    * window as visible.  So there's nothing inherently different about
    * the hotkey button, it just launches the input settings and sets
    * the proper device.
    *
    * There may be a better way than to hard-code (maybe search the
    * name string in the controller somehow?) the values, but for now it
    * will do the job.
    */
    if (hotkeys) {
        deviceBox.setSelection(1);
        deviceChanged();
    } else {
        deviceBox.setSelection(0);
        deviceChanged();
    }

    TopLevelWindow::setVisible();
}

void InputSettings::deviceChanged(void)
{
    mappingList.reset();
    InputMapper::ControllerPort &port = (
        portBox.selection() == 0
        ? (InputMapper::ControllerPort&)inputMapper.port1
        : (InputMapper::ControllerPort&)inputMapper.port2
    );

    InputMapper::Controller &controller =
        (InputMapper::Controller&)*port[deviceBox.selection()];

    for (unsigned i = 0; i < controller.size(); i++) {
        string mapping = controller[i]->mapping;
        if (mapping == "") {
          mapping = "None";
        }
        mappingList.addItem({ controller[i]->name, "\t", mapping });
    }
    mappingList.resizeColumnsToContent();
}

void InputSettings::mappingChanged(void)
{
    InputMapper::ControllerPort &port = (
        portBox.selection() == 0
        ? (InputMapper::ControllerPort&)inputMapper.port1
        : (InputMapper::ControllerPort&)inputMapper.port2
    );

    InputMapper::Controller &controller =
        (InputMapper::Controller&)*port[deviceBox.selection()];

    for (unsigned i = 0; i < controller.size(); i++) {
        string mapping = controller[i]->mapping;
        if (mapping == "") {
            mapping = "None";
        }
        mappingList.setItem(i, { controller[i]->name, "\t", mapping });
    }

    mappingList.resizeColumnsToContent();
}

void InputSettings::assignInput(void)
{
    if (auto position = mappingList.selection()) {
        InputMapper::ControllerPort &port = (
            portBox.selection() == 0
            ? (InputMapper::ControllerPort&)inputMapper.port1
            : (InputMapper::ControllerPort&)inputMapper.port2
        );

        InputMapper::Controller &controller =
            (InputMapper::Controller&)*port[deviceBox.selection()];

        portBox.setEnabled(false);
        deviceBox.setEnabled(false);
        mappingList.setEnabled(false);

        /* flush any pending keypresses */
        inputMapper.poll();
        activeInput = controller[position()];
        setStatusText({ "Set assignment for [", activeInput->name, "] ..." });

        if (dynamic_cast<InputMapper::AnalogInput*>(activeInput)) {
            mouseLeft.setVisible(false);
            mouseMiddle.setVisible(false);
            mouseRight.setVisible(false);
            mouseXaxis.setVisible(true);
            mouseYaxis.setVisible(true);
        } else {
            mouseXaxis.setVisible(false);
            mouseYaxis.setVisible(false);
            mouseLeft.setVisible(true);
            mouseMiddle.setVisible(true);
            mouseRight.setVisible(true);
        }
    }
}

void InputSettings::clearInput(void)
{
    if (auto position = mappingList.selection()) {
        InputMapper::ControllerPort &port = (
            portBox.selection() == 0
            ? (InputMapper::ControllerPort&)inputMapper.port1
            : (InputMapper::ControllerPort&)inputMapper.port2
        );

        InputMapper::Controller &controller =
            (InputMapper::Controller&)*port[deviceBox.selection()];

        controller[position()]->mapping = "";
        inputMapper.bind();
        endAssignment();
    }
}

void InputSettings::setMapping(const string &mapping)
{
    activeInput->mapping = mapping;
    inputMapper.bind();
    endAssignment();
}

void InputSettings::endAssignment(void)
{
    activeInput = 0;
    portBox.setEnabled(true);
    deviceBox.setEnabled(true);
    mappingList.setEnabled(true);
    setStatusText("");
    mouseXaxis.setVisible(false);
    mouseYaxis.setVisible(false);
    mouseLeft.setVisible(false);
    mouseMiddle.setVisible(false);
    mouseRight.setVisible(false);
    mappingChanged();
    mappingList.setFocused();
}

void InputSettings::inputEvent(uint16_t scancode, int16_t value)
{
    /* not remapping any keys right now? */
    if (activeInput == 0) {
        return;
    }

    string mapping = Scancode::encode(scancode);
    if (auto position = strpos(mapping, "::Escape")) {
        return setMapping("");
    }

    if (dynamic_cast<InputMapper::AnalogInput*>(activeInput)) {
    } else if (dynamic_cast<InputMapper::DigitalInput*>(activeInput)) {
        if (Keyboard::isAnyKey(scancode) && value) {
            setMapping(mapping);
        } else if (Mouse::isAnyButton(scancode) && value) {
            activeMouse = Mouse::numberDecode(scancode);
        } else if (Joypad::isAnyHat(scancode) && value) {
            if (value == Joypad::HatUp) {
                setMapping({ mapping, ".Up" });
            } else if (value == Joypad::HatDown) {
                setMapping({ mapping, ".Down" });
            } else if(value == Joypad::HatLeft) {
                setMapping({ mapping, ".Left" });
            } else if(value == Joypad::HatRight) {
                setMapping({ mapping, ".Right" });
            }
        } else if (Joypad::isAnyAxis(scancode)) {
            if (joypadsCalibrated == false) {
                return calibrateJoypads();
            }

            unsigned joypadNumber = Joypad::numberDecode(scancode);
            unsigned axisNumber = Joypad::axisDecode(scancode);
            int16_t calibration = joypadCalibration[joypadNumber][axisNumber];
            if (calibration > -12288 && calibration < +12288 &&
                value < -24576) {
                setMapping({ mapping, ".Lo" });
            } else if (calibration > -12288 && calibration < +12288 &&
                value > +24576) {
                setMapping({ mapping, ".Hi" });
            } else if (calibration <= -12288 && value >= +12288) {
                setMapping({ mapping, ".Hi" });
            } else if (calibration >= +12288 && value <= -12288) {
                setMapping({ mapping, ".Lo" });
            }
        } else if (Joypad::isAnyButton(scancode) && value) {
            setMapping(mapping);
        }
    }
}

void InputSettings::calibrateJoypads(void)
{
    if (joypadsCalibrating == true) {
        return;
    }

    joypadsCalibrating = true;
    MessageWindow::information(*this,
        "Analog joypads must be calibrated prior to use.\n\n"
        "Please move all analog axes, and press all analog buttons.\n"
        "Please do this for every controller you wish to use.\n"
        "Once finished, please let go of all axes and buttons, and press OK."
    );

    inputMapper.poll();
    for (unsigned j = 0; j < Joypad::Count &&j<2; j++) {
        for (unsigned a = 0; a < Joypad::Axes; a++) {
            joypadCalibration[j][a] = inputMapper.value(joypad(j).axis(a));
        }
    }

    joypadsCalibrating = false;
    joypadsCalibrated = true;
}

InputSettings::InputSettings(void)
{
    joypadsCalibrated = false;
    joypadsCalibrating = false;
}
