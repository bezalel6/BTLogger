#include "SDCardManager.hpp"
#include "BluetoothManager.hpp"  // For LogPacket
#include <time.h>

namespace BTLogger {
namespace Core {

// Define CS pin for SD card
#define SD_CS_PIN 5

SDCardManager::SDCardManager()
    : csPin(SD_CS_PIN), logDirectory("/logs"), maxFileSize(1024 * 1024),  // 1MB
      maxFilesPerSession(10),
      currentFileSize(0),
      currentFileNumber(0) {
}

SDCardManager::~SDCardManager() {
    endCurrentSession();
}

bool SDCardManager::initialize() {
    Serial.print("Initializing SD card...");

    // Initialize SPI for SD card
    SPI.begin();

    if (!SD.begin(csPin)) {
        Serial.println(" FAILED");
        return false;
    }

    Serial.println(" OK");

    // Check card type
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        return false;
    }

    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    // Print card size
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    // Ensure log directory exists
    if (!ensureDirectoryExists(logDirectory)) {
        Serial.println("Failed to create log directory");
        return false;
    }

    return true;
}

bool SDCardManager::isCardPresent() const {
    return SD.cardType() != CARD_NONE;
}

bool SDCardManager::startNewSession(const String& deviceName) {
    endCurrentSession();

    if (!isCardPresent()) {
        Serial.println("No SD card present for logging");
        return false;
    }

    currentDeviceName = deviceName;
    currentFileNumber = 1;
    currentFileSize = 0;

    currentSessionFile = generateLogFileName(deviceName, currentFileNumber);

    Serial.printf("Starting new session: %s\n", currentSessionFile.c_str());

    // Create the initial log file
    currentFile = SD.open(currentSessionFile, FILE_WRITE);
    if (!currentFile) {
        Serial.printf("Failed to create log file: %s\n", currentSessionFile.c_str());
        return false;
    }

    // Write session header
    String header = String("# BTLogger Session Started\n");
    header += "# Device: " + deviceName + "\n";
    header += "# Time: " + formatTimestamp(millis()) + "\n";
    header += "# Format: timestamp,level,tag,message\n\n";

    currentFile.print(header);
    currentFile.flush();
    currentFileSize = header.length();

    Serial.printf("Session started: %s\n", currentSessionFile.c_str());
    return true;
}

void SDCardManager::endCurrentSession() {
    if (currentFile) {
        String footer = "\n# Session ended: " + formatTimestamp(millis()) + "\n";
        currentFile.print(footer);
        currentFile.close();
        Serial.printf("Session ended: %s\n", currentSessionFile.c_str());
    }

    currentSessionFile = "";
    currentDeviceName = "";
    currentFileSize = 0;
    currentFileNumber = 0;
}

bool SDCardManager::saveLogToSession(const LogPacket& packet, const String& deviceName) {
    if (!currentFile || currentDeviceName != deviceName) {
        if (!startNewSession(deviceName)) {
            return false;
        }
    }

    // Check if we need to rotate the file
    if (currentFileSize > maxFileSize) {
        if (!rotateLogFile()) {
            return false;
        }
    }

    // Format log entry
    String logEntry = formatTimestamp(packet.timestamp) + ",";
    logEntry += String(packet.level) + ",";
    logEntry += String(packet.tag) + ",";
    logEntry += String(packet.message) + "\n";

    // Write to file
    size_t written = currentFile.print(logEntry);
    currentFile.flush();

    if (written > 0) {
        currentFileSize += written;
        return true;
    }

    Serial.println("Failed to write log entry to SD card");
    return false;
}

std::vector<String> SDCardManager::loadLogFile(const String& path) {
    std::vector<String> lines;

    if (!isCardPresent()) {
        return lines;
    }

    File file = SD.open(path, FILE_READ);
    if (!file) {
        Serial.printf("Failed to open file: %s\n", path.c_str());
        return lines;
    }

    String currentLine = "";
    while (file.available()) {
        char c = file.read();
        if (c == '\n') {
            if (currentLine.length() > 0) {
                lines.push_back(currentLine);
                currentLine = "";
            }
        } else if (c != '\r') {
            currentLine += c;
        }

        // Limit to prevent memory issues
        if (lines.size() > 1000) {
            break;
        }
    }

    // Add the last line if it doesn't end with newline
    if (currentLine.length() > 0) {
        lines.push_back(currentLine);
    }

    file.close();
    Serial.printf("Loaded %d lines from %s\n", lines.size(), path.c_str());
    return lines;
}

bool SDCardManager::saveLogFile(const String& path, const std::vector<String>& lines) {
    if (!isCardPresent()) {
        return false;
    }

    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.printf("Failed to create file: %s\n", path.c_str());
        return false;
    }

    for (const String& line : lines) {
        file.println(line);
    }

    file.close();
    Serial.printf("Saved %d lines to %s\n", lines.size(), path.c_str());
    return true;
}

bool SDCardManager::deleteFile(const String& path) {
    if (!isCardPresent()) {
        return false;
    }

    if (SD.remove(path)) {
        Serial.printf("Deleted file: %s\n", path.c_str());
        return true;
    }

    Serial.printf("Failed to delete file: %s\n", path.c_str());
    return false;
}

bool SDCardManager::createDirectory(const String& path) {
    if (!isCardPresent()) {
        return false;
    }

    if (SD.mkdir(path)) {
        Serial.printf("Created directory: %s\n", path.c_str());
        return true;
    }

    Serial.printf("Failed to create directory: %s\n", path.c_str());
    return false;
}

std::vector<FileInfo> SDCardManager::listDirectory(const String& path) {
    std::vector<FileInfo> files;

    if (!isCardPresent()) {
        return files;
    }

    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) {
        Serial.printf("Failed to open directory: %s\n", path.c_str());
        return files;
    }

    File entry = dir.openNextFile();
    while (entry) {
        String name = entry.name();
        String fullPath = path;
        if (!fullPath.endsWith("/")) {
            fullPath += "/";
        }
        fullPath += name;

        FileInfo info(name, fullPath, entry.size(), entry.isDirectory(), "");
        files.push_back(info);

        entry.close();
        entry = dir.openNextFile();
    }

    dir.close();
    return files;
}

std::vector<FileInfo> SDCardManager::listLogFiles() {
    return listDirectory(logDirectory);
}

FileInfo SDCardManager::getFileInfo(const String& path) {
    FileInfo info;

    if (!isCardPresent()) {
        return info;
    }

    File file = SD.open(path);
    if (!file) {
        return info;
    }

    info.name = file.name();
    info.path = path;
    info.size = file.size();
    info.isDirectory = file.isDirectory();
    info.lastModified = formatTimestamp(file.getLastWrite());

    file.close();
    return info;
}

unsigned long SDCardManager::getTotalSpace() const {
    if (!isCardPresent()) {
        return 0;
    }
    return SD.totalBytes();
}

unsigned long SDCardManager::getUsedSpace() const {
    if (!isCardPresent()) {
        return 0;
    }
    return SD.usedBytes();
}

unsigned long SDCardManager::getFreeSpace() const {
    if (!isCardPresent()) {
        return 0;
    }
    return SD.totalBytes() - SD.usedBytes();
}

String SDCardManager::generateSessionFileName(const String& deviceName) {
    return generateLogFileName(deviceName, 1);
}

String SDCardManager::generateLogFileName(const String& deviceName, int fileNumber) {
    String safeName = deviceName;
    safeName.replace(" ", "_");
    safeName.replace(".", "_");

    String timestamp = formatTimestamp(millis());
    timestamp.replace(":", "-");
    timestamp.replace(" ", "_");

    String filename = logDirectory + "/" + safeName + "_" + timestamp;
    if (fileNumber > 1) {
        filename += "_" + String(fileNumber);
    }
    filename += ".log";

    return filename;
}

bool SDCardManager::rotateLogFile() {
    if (currentFileNumber >= maxFilesPerSession) {
        Serial.println("Maximum files per session reached");
        return false;
    }

    // Close current file
    closeCurrentFile();

    // Create new file
    currentFileNumber++;
    currentSessionFile = generateLogFileName(currentDeviceName, currentFileNumber);

    currentFile = SD.open(currentSessionFile, FILE_WRITE);
    if (!currentFile) {
        Serial.printf("Failed to create rotated log file: %s\n", currentSessionFile.c_str());
        return false;
    }

    // Write rotation header
    String header = String("# Log file rotated\n");
    header += "# File: " + String(currentFileNumber) + " of session\n";
    header += "# Time: " + formatTimestamp(millis()) + "\n\n";

    currentFile.print(header);
    currentFile.flush();
    currentFileSize = header.length();

    Serial.printf("Rotated to new log file: %s\n", currentSessionFile.c_str());
    return true;
}

String SDCardManager::formatTimestamp(unsigned long timestamp) {
    // Convert milliseconds to seconds
    time_t time = timestamp / 1000;

    // Simple timestamp format
    return String(time);
}

String SDCardManager::formatFileSize(unsigned long bytes) {
    if (bytes < 1024) {
        return String(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return String(bytes / 1024) + " KB";
    } else {
        return String(bytes / (1024 * 1024)) + " MB";
    }
}

bool SDCardManager::ensureDirectoryExists(const String& path) {
    File dir = SD.open(path);
    if (dir && dir.isDirectory()) {
        dir.close();
        return true;
    }

    if (dir) {
        dir.close();
    }

    return createDirectory(path);
}

void SDCardManager::closeCurrentFile() {
    if (currentFile) {
        String footer = "# File closed: " + formatTimestamp(millis()) + "\n";
        currentFile.print(footer);
        currentFile.close();
    }
}

}  // namespace Core
}  // namespace BTLogger