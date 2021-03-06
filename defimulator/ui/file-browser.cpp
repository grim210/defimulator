#include <ui/file-browser.h>

FileBrowser fileBrowser;

void FileBrowser::create(void)
{
    Window::create(0, 0, 256, 256);
    application.addWindow(this, "FileBrowser", "160,160");

    unsigned x = 5, y = 5, height = Style::TextBoxHeight;
    unsigned browsewidth = 60, backwidth = 40;

    pathBox.create(*this, x, y, 630 - browsewidth - backwidth - 10,
        height);
    browseButton.create(*this, x + 630 - browsewidth - backwidth - 5, y,
        browsewidth, height, "Browse");
    upButton.create(*this, x + 630 - backwidth, y, backwidth,
        height, "Back");
    y += height + 5;

    contentsBox.create(*this, x, y, 630, 350); y += 350 + 5;

    setGeometry(0, 0, 640, y);

    pathBox.onActivate = []() {
        string path = fileBrowser.pathBox.text();
        path.transform("\\", "/");
        if (strend(path, "/") == false) {
            path.append("/");
        }
        fileBrowser.setFolder(path);
    };

    browseButton.onTick = { &FileBrowser::folderBrowse, this };
    upButton.onTick = { &FileBrowser::folderUp, this };
    contentsBox.onActivate = { &FileBrowser::fileActivate, this };
}

void FileBrowser::fileOpen(FileBrowser::Mode requestedMode,
    function<void (string)> requestedCallback)
{
    callback = requestedCallback;
    if (mode == requestedMode && folder == config.path.current) {
        setVisible();
        contentsBox.setFocused();
        return;
    }

    filters.reset();
    switch (mode = requestedMode) {
    case Mode::Cartridge:
        setTitle("Load Cartridge");
        filters.append(".sfc");
        break;
    case Mode::Satellaview:
        setTitle("Load Satellaview Cartridge");
        filters.append(".bs");
        break;
    case Mode::SufamiTurbo:
        setTitle("Load Sufami Turbo Cartridge");
        filters.append(".st");
        break;
    case Mode::GameBoy:
        setTitle("Load Game Boy Cartridge");
        filters.append(".gb");
        filters.append(".gbc");
        filters.append(".sgb");
        break;
    case Mode::Filter:
        setTitle("Load Video Filter");
        filters.append(".so");
        filters.append(".dll");
        break;
    case Mode::Shader:
        setTitle("Load Pixel Shader");
        filters.append(".shader");
        break;
    }

    setVisible(false);
    setFolder(config.path.current);
    setVisible(true);
    contentsBox.setFocused();
}

void FileBrowser::setFolder(const string &pathname)
{
    contentsBox.reset();
    contents.reset();

    folder = pathname;
    folder.transform("\\", "/");
    pathBox.setText(folder);
    lstring contentsList = directory::contents(folder);
    nall_foreach(item, contentsList) {
        if (strend(item, "/")) {
            contents.append(item);
        } else {
            nall_foreach (filter, filters) {
                if (strend(item, filter)) {
                    contents.append(item);
                    break;
                }
            }
        }
    }

    nall_foreach (item, contents) {
        contentsBox.addItem(item);
    }

    contentsBox.setSelection(0);
    contentsBox.setFocused();
}

void FileBrowser::folderBrowse(void)
{
    string pathname = OS::folderSelect(*this, folder);
    if (pathname != "") {
        setFolder(pathname);
    }
}

void FileBrowser::folderUp(void)
{
    string path = folder;
    path.rtrim<1>("/");
    if (path != "") {
        setFolder(dir(path));
    }
}

void FileBrowser::fileActivate(void)
{
    if (auto position = contentsBox.selection()) {
        string filename = contents[position()];
        if (strend(filename, "/")) {
            string cartridgeName = cartridgeFolder(filename);
            if (cartridgeName == "") {
                setFolder({ folder, filename });
            } else {
                loadFile({ folder, cartridgeName });
            }
        } else {
            loadFile({ folder, filename });
        }
    }
}

string FileBrowser::cartridgeFolder(const string &pathname)
{
    if (strend(pathname, ".sfc/") == false) {
        return "";
    }

    lstring list = directory::files(string(folder, "/", pathname));
    string filename;
    nall_foreach (item, list) {
        if (strend(item, ".sfc")) {
            if (filename != "") {
                /* more than one cartridge in this folder. */
                return "";
            }
            filename = item;
        }
    }

    return { pathname, filename };
}

void FileBrowser::loadFile(const string &filename)
{
    setVisible(false);
    config.path.current = folder;
    if (callback) {
        callback(filename);
    }
}
