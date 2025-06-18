#pragma once

#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <SPI.h>
#include <vector>
#include "BluetoothManager.hpp"

// File info structure
struct FileInfo {
    String name;
    String path;
    size_t size;
    bool isDirectory;
    unsigned long lastModified;

    FileInfo() : size(0), isDirectory(false), lastModified(0) {}
    FileInfo(const String& n, const String& p, size_t s, bool dir, unsigned long mod)
        : name(n), path(p), size(s), isDirectory(dir), lastModified(mod) {}
};

// Log session info
struct LogSession {
    String filename;
    String deviceName;
    unsigned long startTime;
    unsigned long endTime;
    size_t logCount;

    LogSession() : startTime(0), endTime(0), logCount(0) {}
};

class SDCardManager {
   public:
    SDCardManager();
    ~SDCardManager();

    // Core functionality
    bool initialize();
    bool isCardPresent() const { return cardPresent; }

    // Session management
    bool startNewSession(const String& deviceName = "");
    bool endCurrentSession();
    bool saveLogToSession(const LogPacket& packet, const String& deviceName);
    String getCurrentSessionFile() const { return currentSessionFile; }

    // File operations
    std::vector<FileInfo> listDirectory(const String& path = "/");
    std::vector<LogSession> getLogSessions();
    bool deleteFile(const String& path);
    bool createDirectory(const String& path);
    String readFile(const String& path);
    bool writeFile(const String& path, const String& content);

    // Log file operations
    std::vector<String> loadLogFile(const String& filename);
    bool exportLogs(const String& sessionFile, const String& format = "txt");

    // Utility
    size_t getFreeSpace();
    size_t getTotalSpace();
    bool formatCard();
    void cleanupOldSessions(int keepDays = 30);

    // Configuration
    void setAutoSave(bool enabled) { autoSaveEnabled = enabled; }
    void setMaxSessionSize(size_t maxSize) { maxSessionSizeBytes = maxSize; }

   private:
    // State
    bool cardPresent;
    bool autoSaveEnabled;
    String currentSessionFile;
    File currentLogFile;
    size_t maxSessionSizeBytes;
    size_t currentSessionSize;
    SPIClass* spi;

    // Cached card info to avoid SPI conflicts
    size_t cachedTotalSpace;
    size_t cachedUsedSpace;

    // Constants
    static const String LOG_DIR;
    static const String SESSION_DIR;
    static const String CONFIG_DIR;

    // Internal methods
    String generateSessionFilename(const String& deviceName = "");
    String formatLogEntry(const LogPacket& packet, const String& deviceName);
    String formatTimestamp(unsigned long timestamp);
    String getLevelString(uint8_t level);
    bool ensureDirectoryExists(const String& path);
    void updateSessionMetadata();
};