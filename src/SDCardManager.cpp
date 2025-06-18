#include "SDCardManager.hpp"
#include <SPI.h>

// Constants
const String SDCardManager::LOG_DIR = "/logs";
const String SDCardManager::SESSION_DIR = "/logs/sessions";
const String SDCardManager::CONFIG_DIR = "/logs/config";

// SD card pin definitions (from main.cpp)
#define SD_SCK 18
#define SD_MISO 19
#define SD_MOSI 23
#define SD_CS 5

SDCardManager::SDCardManager()
    : cardPresent(false), autoSaveEnabled(true), maxSessionSizeBytes(10 * 1024 * 1024)  // 10MB default
      ,
      currentSessionSize(0),
      spi(nullptr),
      cachedTotalSpace(0),
      cachedUsedSpace(0) {
}

SDCardManager::~SDCardManager() {
    endCurrentSession();
    if (spi) {
        delete spi;
        spi = nullptr;
    }
}

bool SDCardManager::initialize() {
    Serial.println("Initializing SD Card Manager...");

    // Create SPI instance if not already created
    if (!spi) {
        spi = new SPIClass(VSPI);
    }

    // Initialize SD card using VSPI
    if (!SD.begin(SS, *spi, 80000000)) {
        Serial.println("SD Card initialization failed");
        cardPresent = false;
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        cardPresent = false;
        return false;
    }

    cardPresent = true;
    Serial.println("SD Card initialized successfully");

    // Print card type
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

    // Create directory structure
    ensureDirectoryExists(LOG_DIR);
    ensureDirectoryExists(SESSION_DIR);
    ensureDirectoryExists(CONFIG_DIR);

    // Cache card info to avoid SPI conflicts later
    cachedTotalSpace = SD.cardSize();
    cachedUsedSpace = 0;  // We'll calculate this if needed

    // Print card info
    Serial.printf("SD Card Size: %llu MB\n", cachedTotalSpace / (1024 * 1024));
    Serial.printf("Total Space: %llu MB\n", cachedTotalSpace / (1024 * 1024));
    Serial.printf("Used Space: %llu MB\n", cachedUsedSpace / (1024 * 1024));

    return true;
}

bool SDCardManager::startNewSession(const String& deviceName) {
    if (!cardPresent) {
        Serial.println("Cannot start session - SD card not present");
        return false;
    }

    // End current session if active
    endCurrentSession();

    // Generate new session filename
    currentSessionFile = generateSessionFilename(deviceName);
    String fullPath = SESSION_DIR + "/" + currentSessionFile;

    Serial.printf("Starting new log session: %s\n", fullPath.c_str());

    // Open file for writing
    currentLogFile = SD.open(fullPath, FILE_WRITE);
    if (!currentLogFile) {
        Serial.printf("Failed to create session file: %s\n", fullPath.c_str());
        return false;
    }

    // Write session header
    currentLogFile.printf("# BTLogger Session Log\n");
    currentLogFile.printf("# Session: %s\n", currentSessionFile.c_str());
    currentLogFile.printf("# Device: %s\n", deviceName.c_str());
    currentLogFile.printf("# Started: %s\n", formatTimestamp(millis()).c_str());
    currentLogFile.printf("# Format: [TIMESTAMP] [LEVEL] [TAG] MESSAGE\n");
    currentLogFile.printf("# =====================================\n\n");
    currentLogFile.flush();

    currentSessionSize = currentLogFile.size();

    Serial.printf("Session started successfully: %s\n", currentSessionFile.c_str());
    return true;
}

bool SDCardManager::endCurrentSession() {
    if (!currentLogFile) {
        return false;
    }

    Serial.printf("Ending log session: %s\n", currentSessionFile.c_str());

    // Write session footer
    currentLogFile.printf("\n# Session ended: %s\n", formatTimestamp(millis()).c_str());
    currentLogFile.printf("# Total size: %zu bytes\n", currentSessionSize);
    currentLogFile.close();

    updateSessionMetadata();

    currentSessionFile = "";
    currentSessionSize = 0;

    return true;
}

bool SDCardManager::saveLogToSession(const LogPacket& packet, const String& deviceName) {
    if (!cardPresent || !autoSaveEnabled) {
        return false;
    }

    // Start new session if none active
    if (!currentLogFile) {
        if (!startNewSession(deviceName)) {
            return false;
        }
    }

    // Check session size limit
    if (currentSessionSize > maxSessionSizeBytes) {
        endCurrentSession();
        if (!startNewSession(deviceName)) {
            return false;
        }
    }

    // Format and write log entry
    String logEntry = formatLogEntry(packet, deviceName);
    size_t written = currentLogFile.print(logEntry);
    currentLogFile.flush();

    if (written > 0) {
        currentSessionSize += written;
        return true;
    }

    return false;
}

std::vector<FileInfo> SDCardManager::listDirectory(const String& path) {
    std::vector<FileInfo> files;

    if (!cardPresent) {
        return files;
    }

    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) {
        return files;
    }

    File file = dir.openNextFile();
    while (file) {
        FileInfo info;
        info.name = file.name();
        info.path = path + "/" + info.name;
        info.size = file.size();
        info.isDirectory = file.isDirectory();
        info.lastModified = 0;  // Not available in basic FS

        files.push_back(info);
        file = dir.openNextFile();
    }

    dir.close();
    return files;
}

std::vector<LogSession> SDCardManager::getLogSessions() {
    std::vector<LogSession> sessions;

    if (!cardPresent) {
        return sessions;
    }

    std::vector<FileInfo> sessionFiles = listDirectory(SESSION_DIR);

    for (const auto& fileInfo : sessionFiles) {
        if (fileInfo.isDirectory || !fileInfo.name.endsWith(".log")) {
            continue;
        }

        LogSession session;
        session.filename = fileInfo.name;

        // Parse filename for metadata (format: YYYY-MM-DD_HH-MM-SS_devicename.log)
        int firstUnderscore = fileInfo.name.indexOf('_');
        int secondUnderscore = fileInfo.name.indexOf('_', firstUnderscore + 1);
        int dotIndex = fileInfo.name.lastIndexOf('.');

        if (firstUnderscore > 0 && secondUnderscore > firstUnderscore && dotIndex > secondUnderscore) {
            session.deviceName = fileInfo.name.substring(secondUnderscore + 1, dotIndex);
        }

        session.logCount = 0;  // Would need to count lines in file
        sessions.push_back(session);
    }

    return sessions;
}

bool SDCardManager::deleteFile(const String& path) {
    if (!cardPresent) {
        return false;
    }

    return SD.remove(path);
}

bool SDCardManager::createDirectory(const String& path) {
    if (!cardPresent) {
        return false;
    }

    return SD.mkdir(path);
}

String SDCardManager::readFile(const String& path) {
    if (!cardPresent) {
        return "";
    }

    File file = SD.open(path);
    if (!file) {
        return "";
    }

    String content = file.readString();
    file.close();
    return content;
}

bool SDCardManager::writeFile(const String& path, const String& content) {
    if (!cardPresent) {
        return false;
    }

    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        return false;
    }

    size_t written = file.print(content);
    file.close();
    return written == content.length();
}

std::vector<String> SDCardManager::loadLogFile(const String& filename) {
    std::vector<String> lines;

    if (!cardPresent) {
        return lines;
    }

    String fullPath = SESSION_DIR + "/" + filename;
    File file = SD.open(fullPath);
    if (!file) {
        return lines;
    }

    String line;
    while (file.available()) {
        char c = file.read();
        if (c == '\n') {
            if (!line.startsWith("#") && line.length() > 0) {  // Skip comments and empty lines
                lines.push_back(line);
            }
            line = "";
        } else if (c != '\r') {
            line += c;
        }
    }

    // Add last line if it doesn't end with newline
    if (!line.startsWith("#") && line.length() > 0) {
        lines.push_back(line);
    }

    file.close();
    return lines;
}

bool SDCardManager::exportLogs(const String& sessionFile, const String& format) {
    // Simple implementation - just copy the file for now
    // Could add CSV, JSON export formats later
    return true;
}

size_t SDCardManager::getFreeSpace() {
    if (!cardPresent) {
        return 0;
    }
    return cachedTotalSpace - cachedUsedSpace;
}

size_t SDCardManager::getTotalSpace() {
    if (!cardPresent) {
        return 0;
    }
    return cachedTotalSpace;
}

bool SDCardManager::formatCard() {
    // Note: Basic SD library doesn't support formatting
    // This would require more advanced filesystem operations
    Serial.println("Format not supported with basic SD library");
    return false;
}

void SDCardManager::cleanupOldSessions(int keepDays) {
    if (!cardPresent) {
        return;
    }

    // Simple cleanup - delete files older than keepDays
    // This is a basic implementation
    unsigned long cutoffTime = millis() - (keepDays * 24 * 60 * 60 * 1000UL);

    std::vector<FileInfo> files = listDirectory(SESSION_DIR);
    for (const auto& file : files) {
        if (!file.isDirectory && file.name.endsWith(".log")) {
            // For simplicity, we'll skip time-based deletion for now
            // In a full implementation, we'd parse file modification time
        }
    }
}

String SDCardManager::generateSessionFilename(const String& deviceName) {
    // Format: YYYY-MM-DD_HH-MM-SS_devicename.log
    unsigned long timestamp = millis();

    // Simple timestamp conversion (not real time - would need RTC)
    int seconds = (timestamp / 1000) % 60;
    int minutes = (timestamp / (1000 * 60)) % 60;
    int hours = (timestamp / (1000 * 60 * 60)) % 24;
    int days = timestamp / (1000 * 60 * 60 * 24);

    String filename = String(days) + "-" +
                      String(hours, DEC) + "-" +
                      String(minutes, DEC) + "-" +
                      String(seconds, DEC);

    if (!deviceName.isEmpty()) {
        filename += "_" + deviceName;
    }

    filename += ".log";
    return filename;
}

String SDCardManager::formatLogEntry(const LogPacket& packet, const String& deviceName) {
    String entry = "[" + formatTimestamp(packet.timestamp) + "] ";
    entry += "[" + getLevelString(packet.level) + "] ";
    entry += "[" + String(packet.tag) + "] ";
    entry += String(packet.message);
    entry += " {" + deviceName + "}\n";
    return entry;
}

String SDCardManager::formatTimestamp(unsigned long timestamp) {
    // Simple millisecond timestamp - in real implementation would use RTC
    return String(timestamp);
}

String SDCardManager::getLevelString(uint8_t level) {
    switch (level) {
        case 0:
            return "DEBUG";
        case 1:
            return "INFO ";
        case 2:
            return "WARN ";
        case 3:
            return "ERROR";
        default:
            return "UNKN ";
    }
}

bool SDCardManager::ensureDirectoryExists(const String& path) {
    if (!cardPresent) {
        return false;
    }

    File dir = SD.open(path);
    if (dir && dir.isDirectory()) {
        dir.close();
        return true;
    }

    if (dir) {
        dir.close();
    }

    return SD.mkdir(path);
}

void SDCardManager::updateSessionMetadata() {
    // In a full implementation, this would update a metadata file
    // with session information for quick access
}