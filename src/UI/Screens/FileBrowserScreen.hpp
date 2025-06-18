#pragma once

#include "../Screen.hpp"
#include "../Widgets/Button.hpp"
#include "../../Core/SDCardManager.hpp"
#include <vector>

namespace BTLogger {
namespace UI {
namespace Screens {

/**
 * File browser screen for SD card files
 */
class FileBrowserScreen : public Screen {
   public:
    FileBrowserScreen();
    virtual ~FileBrowserScreen();

    void activate() override;
    void deactivate() override;
    void update() override;
    void handleTouch(int x, int y, bool touched) override;
    void cleanup() override;

    // SD Card integration
    void setSDCardManager(Core::SDCardManager* sdManager);

   private:
    struct FileInfo {
        String name;
        String path;
        bool isDirectory;
        size_t size;

        FileInfo(const String& n, const String& p, bool dir, size_t s = 0)
            : name(n), path(p), isDirectory(dir), size(s) {}
    };

    // UI Elements
    Widgets::Button* backButton;
    Widgets::Button* refreshButton;
    Widgets::Button* deleteButton;

    // File list buttons (dynamically created)
    std::vector<Widgets::Button*> fileButtons;

    // File data
    std::vector<FileInfo> files;
    Core::SDCardManager* sdCardManager;
    String currentPath;
    int scrollOffset;
    int maxVisibleFiles;
    bool lastTouchState;
    int selectedFileIndex;

    // Constants
    static const int MAX_FILES = 50;
    static const int FILE_BUTTON_HEIGHT = 30;

    void createControlButtons();
    void refreshFileList();
    void updateFileList();
    void drawFileList();
    void handleScrolling(int x, int y, bool wasTapped);
    void selectFile(int index);
    void deleteSelectedFile();
    void navigateToDirectory(const String& dirPath);
    void scrollUp();
    void scrollDown();
    String formatFileInfo(const FileInfo& file);
    String formatFileSize(size_t bytes);
};

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger