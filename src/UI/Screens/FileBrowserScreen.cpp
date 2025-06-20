#include "FileBrowserScreen.hpp"
#include "../UIScale.hpp"
#include "../ScreenManager.hpp"
#include "../TouchManager.hpp"

namespace BTLogger {
namespace UI {
namespace Screens {

FileBrowserScreen::FileBrowserScreen() : Screen("FileBrowser"),
                                         backButton(nullptr),
                                         refreshButton(nullptr),
                                         deleteButton(nullptr),
                                         sdCardManager(nullptr),
                                         currentPath("/"),
                                         scrollOffset(0),
                                         maxVisibleFiles(0),
                                         lastTouchState(false),
                                         selectedFileIndex(-1) {
}

FileBrowserScreen::~FileBrowserScreen() {
    cleanup();
}

void FileBrowserScreen::activate() {
    Screen::activate();

    if (!backButton) {
        createControlButtons();
    }

    refreshFileList();
    ScreenManager::setStatusText("File Browser - SD Card Files");
}

void FileBrowserScreen::deactivate() {
    Screen::deactivate();
}

void FileBrowserScreen::update() {
    if (!active) return;

    if (needsRedraw) {
        drawFileList();
        needsRedraw = false;
    }

    // Update buttons
    if (backButton) backButton->update();
    if (refreshButton) refreshButton->update();
    if (deleteButton) deleteButton->update();

    // Update file buttons
    for (auto button : fileButtons) {
        if (button) button->update();
    }
}

void FileBrowserScreen::handleTouch(int x, int y, bool touched) {
    if (!active) return;

    bool wasTapped = TouchManager::wasTapped();

    if (wasTapped) {
        handleScrolling(x, y, wasTapped);
    }

    if (touched || lastTouchState) {
        if (backButton) backButton->handleTouch(x, y, touched);
        if (refreshButton) refreshButton->handleTouch(x, y, touched);
        if (deleteButton) deleteButton->handleTouch(x, y, touched);

        // Handle file buttons
        for (auto button : fileButtons) {
            if (button) {
                button->handleTouch(x, y, touched);
            }
        }

        lastTouchState = touched;
    }
}

void FileBrowserScreen::cleanup() {
    delete backButton;
    delete refreshButton;
    delete deleteButton;

    for (auto button : fileButtons) {
        delete button;
    }
    fileButtons.clear();

    backButton = nullptr;
    refreshButton = nullptr;
    deleteButton = nullptr;
    files.clear();
}

void FileBrowserScreen::setSDCardManager(Core::SDCardManager* sdManager) {
    sdCardManager = sdManager;
}

void FileBrowserScreen::createControlButtons() {
    if (!lcd) return;

    int buttonHeight = UIScale::scale(35);
    int buttonY = UIScale::scale(15);

    // Calculate button widths to divide the full screen width
    int totalWidth = lcd->width();
    int buttonCount = 3;  // BACK, REFRESH, DELETE
    int buttonWidth = totalWidth / buttonCount;

    int currentX = 0;

    // Create back button - takes 1/3 of width
    backButton = new Widgets::Button(*lcd, currentX, buttonY,
                                     buttonWidth, buttonHeight, "BACK");
    backButton->setCallback([this]() {
        Serial.println("Back button pressed in FileBrowser");
        goBack();
    });
    currentX += buttonWidth;

    // Create refresh button - takes 1/3 of width
    refreshButton = new Widgets::Button(*lcd, currentX, buttonY,
                                        buttonWidth, buttonHeight, "REFRESH");
    refreshButton->setCallback([this]() {
        refreshFileList();
    });
    currentX += buttonWidth;

    // Create delete button - takes remaining width (handles any rounding)
    int remainingWidth = totalWidth - currentX;
    deleteButton = new Widgets::Button(*lcd, currentX, buttonY,
                                       remainingWidth, buttonHeight, "DELETE");
    deleteButton->setCallback([this]() {
        deleteSelectedFile();
    });

    Serial.printf("FileBrowser buttons: BACK(%d-%d), REFRESH(%d-%d), DELETE(%d-%d), screen width: %d\n",
                  backButton->getX(), backButton->getX() + backButton->getWidth(),
                  refreshButton->getX(), refreshButton->getX() + refreshButton->getWidth(),
                  deleteButton->getX(), deleteButton->getX() + deleteButton->getWidth(),
                  lcd->width());
}

void FileBrowserScreen::refreshFileList() {
    if (!sdCardManager) {
        ScreenManager::setStatusText("SD Card not available");
        return;
    }

    if (!sdCardManager->isCardPresent()) {
        files.clear();
        updateFileList();
        markForRedraw();
        ScreenManager::setStatusText("SD Card not inserted");
        return;
    }

    // Get file list from SD card manager
    files.clear();

    // Add example files for demonstration
    files.emplace_back("logs", "/logs", true);
    files.emplace_back("config.txt", "/config.txt", false, 1024);
    files.emplace_back("session_001.log", "/logs/session_001.log", false, 15360);
    files.emplace_back("session_002.log", "/logs/session_002.log", false, 8192);
    files.emplace_back("backup", "/backup", true);
    files.emplace_back("settings.json", "/settings.json", false, 512);

    updateFileList();
    markForRedraw();
    ScreenManager::setStatusText(String("Found ") + files.size() + " items");
}

void FileBrowserScreen::updateFileList() {
    // Clear existing file buttons
    for (auto button : fileButtons) {
        delete button;
    }
    fileButtons.clear();

    // Calculate layout
    int startY = HEADER_HEIGHT + UIScale::scale(10);
    int buttonHeight = UIScale::scale(FILE_BUTTON_HEIGHT);
    int buttonSpacing = UIScale::scale(35);
    int buttonWidth = lcd->width() - UIScale::scale(20);

    maxVisibleFiles = (lcd->height() - HEADER_HEIGHT - FOOTER_HEIGHT - UIScale::scale(20)) / buttonSpacing;

    // Create buttons for visible files
    for (size_t i = 0; i < files.size() && i < MAX_FILES; i++) {
        int visibleIndex = i - scrollOffset;
        if (visibleIndex >= 0 && visibleIndex < maxVisibleFiles) {
            int buttonY = startY + (visibleIndex * buttonSpacing);

            String buttonText = formatFileInfo(files[i]);
            auto fileButton = new Widgets::Button(*lcd, UIScale::scale(10), buttonY,
                                                  buttonWidth, buttonHeight, buttonText);

            // Capture file index for callback
            size_t fileIndex = i;

            fileButton->setCallback([this, fileIndex]() {
                selectFile(fileIndex);
            });

            // Color based on file type and selection
            if ((int)i == selectedFileIndex) {
                fileButton->setColors(0xFFE0, 0xFFE8, 0x8410, 0x0000);  // Yellow when selected
            } else if (files[i].isDirectory) {
                fileButton->setColors(0x07FF, 0x07F8, 0x8410, 0x0000);  // Cyan for directories
            } else {
                fileButton->setColors(0x8410, 0x8418, 0x4208, 0xFFFF);  // Gray for files
            }

            fileButtons.push_back(fileButton);
        }
    }
}

void FileBrowserScreen::drawFileList() {
    if (!lcd) return;

    // Clear entire screen first
    lcd->fillScreen(0x0000);

    // Draw header with control buttons
    if (backButton) backButton->draw();
    if (refreshButton) refreshButton->draw();
    if (deleteButton) deleteButton->draw();

    // Draw header line
    lcd->drawFastHLine(0, HEADER_HEIGHT - 1, lcd->width(), 0x8410);

    int infoAreaY = HEADER_HEIGHT;
    int infoAreaHeight = lcd->height() - HEADER_HEIGHT - FOOTER_HEIGHT;

    if (files.empty()) {
        lcd->setTextColor(0x8410);  // Gray
        lcd->setTextSize(UIScale::getGeneralTextSize());
        lcd->setCursor(UIScale::scale(10), infoAreaY + UIScale::scale(20));

        if (!sdCardManager) {
            lcd->print("SD Card Manager not available");
        } else if (!sdCardManager->isCardPresent()) {
            lcd->print("SD Card not inserted");
        } else {
            lcd->print("No files found");
            lcd->setCursor(UIScale::scale(10), infoAreaY + UIScale::scale(40));
            lcd->print("Directory is empty");
        }
        return;
    }

    // Draw file buttons
    for (auto button : fileButtons) {
        if (button) {
            button->draw();
        }
    }

    // Draw scroll indicators
    if (files.size() > maxVisibleFiles) {
        int indicatorX = lcd->width() - UIScale::scale(10);

        if (scrollOffset > 0) {
            lcd->setTextColor(0xFFFF);
            lcd->setTextSize(UIScale::getGeneralTextSize());
            lcd->setCursor(indicatorX, infoAreaY + UIScale::scale(5));
            lcd->print("^");
        }

        if (scrollOffset < (int)files.size() - maxVisibleFiles) {
            lcd->setTextColor(0xFFFF);
            lcd->setTextSize(UIScale::getGeneralTextSize());
            lcd->setCursor(indicatorX, lcd->height() - FOOTER_HEIGHT - UIScale::scale(15));
            lcd->print("v");
        }
    }
}

void FileBrowserScreen::selectFile(int index) {
    if (index < 0 || index >= (int)files.size()) return;

    selectedFileIndex = index;
    const FileInfo& file = files[index];

    Serial.printf("Selected file: %s\n", file.name.c_str());

    if (file.isDirectory) {
        // Navigate to directory (placeholder)
        ScreenManager::setStatusText("Dir: " + file.name);
    } else {
        // Show file info
        String info = file.name + " (" + formatFileSize(file.size) + ")";
        ScreenManager::setStatusText(info);
    }

    updateFileList();
    markForRedraw();
}

void FileBrowserScreen::deleteSelectedFile() {
    if (selectedFileIndex < 0 || selectedFileIndex >= (int)files.size()) {
        ScreenManager::setStatusText("No file selected");
        return;
    }

    const FileInfo& file = files[selectedFileIndex];

    if (sdCardManager) {
        // Placeholder for actual deletion
        Serial.printf("Deleting file: %s\n", file.path.c_str());
        ScreenManager::setStatusText("File deleted: " + file.name);

        // Remove from list and refresh
        files.erase(files.begin() + selectedFileIndex);
        selectedFileIndex = -1;
        updateFileList();
        markForRedraw();
    } else {
        ScreenManager::setStatusText("Cannot delete - SD unavailable");
    }
}

// Helper function to clip text with ellipsis if it's too long
String FileBrowserScreen::clipText(const String& text, int maxWidth, int textSize) {
    if (!lcd) return text;

    int textWidth = UIScale::calculateTextWidth(text, textSize);
    if (textWidth <= maxWidth) {
        return text;  // Text fits, no clipping needed
    }

    // Calculate how many characters we can fit with "..." at the end
    int ellipsisWidth = UIScale::calculateTextWidth("...", textSize);
    int availableWidth = maxWidth - ellipsisWidth;

    if (availableWidth <= 0) {
        return "...";  // Not enough space for anything
    }

    // Binary search to find the maximum number of characters that fit
    int left = 0, right = text.length();
    int bestFit = 0;

    while (left <= right) {
        int mid = (left + right) / 2;
        int midWidth = UIScale::calculateTextWidth(text.substring(0, mid), textSize);

        if (midWidth <= availableWidth) {
            bestFit = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    if (bestFit == 0) {
        return "...";
    }

    return text.substring(0, bestFit) + "...";
}

String FileBrowserScreen::formatFileInfo(const FileInfo& file) {
    String prefix = file.isDirectory ? "[DIR] " : "";
    String suffix = file.isDirectory ? "" : " (" + formatFileSize(file.size) + ")";

    // Calculate available width for file name
    int buttonWidth = lcd->width() - UIScale::scale(20);  // Account for margins
    int prefixWidth = UIScale::calculateTextWidth(prefix, UIScale::getButtonTextSize());
    int suffixWidth = UIScale::calculateTextWidth(suffix, UIScale::getButtonTextSize());
    int availableForName = buttonWidth - prefixWidth - suffixWidth - UIScale::scale(16);  // Account for button padding

    // Clip file name if necessary
    String clippedName = clipText(file.name, availableForName, UIScale::getButtonTextSize());

    return prefix + clippedName + suffix;
}

String FileBrowserScreen::formatFileSize(size_t bytes) {
    if (bytes < 1024) {
        return String(bytes) + "B";
    } else if (bytes < 1024 * 1024) {
        return String(bytes / 1024) + "KB";
    } else {
        return String(bytes / (1024 * 1024)) + "MB";
    }
}

void FileBrowserScreen::handleScrolling(int x, int y, bool wasTapped) {
    if (!wasTapped || files.size() <= maxVisibleFiles) return;

    int infoAreaY = HEADER_HEIGHT;
    int infoAreaHeight = lcd->height() - HEADER_HEIGHT - FOOTER_HEIGHT;

    if (y >= infoAreaY && y < lcd->height() - FOOTER_HEIGHT) {
        if (y < infoAreaY + infoAreaHeight / 2) {
            scrollUp();
        } else {
            scrollDown();
        }
    }
}

void FileBrowserScreen::scrollUp() {
    if (scrollOffset > 0) {
        scrollOffset--;
        updateFileList();
        markForRedraw();
    }
}

void FileBrowserScreen::scrollDown() {
    int maxScroll = std::max(0, (int)files.size() - maxVisibleFiles);
    if (scrollOffset < maxScroll) {
        scrollOffset++;
        updateFileList();
        markForRedraw();
    }
}

void FileBrowserScreen::navigateToDirectory(const String& dirPath) {
    // Placeholder for directory navigation
    currentPath = dirPath;
    refreshFileList();
}

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger