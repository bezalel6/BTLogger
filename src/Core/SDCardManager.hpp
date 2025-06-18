#pragma once

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <vector>
#include <functional>

namespace BTLogger {
namespace Core {

// Forward declaration for LogPacket
struct LogPacket;

// File structure for browsing
struct FileInfo {
    String name;
    String path;
    unsigned long size;
    bool isDirectory;
    String lastModified;

    FileInfo() : size(0), isDirectory(false) {}
    FileInfo(const String& n, const String& p, unsigned long s, bool isDir, const String& modified)
        : name(n), path(p), size(s), isDirectory(isDir), lastModified(modified) {}
};

class SDCardManager {
   public:
    SDCardManager();
    ~SDCardManager();

    // Initialization
    bool initialize();
    bool isCardPresent() const;

    // Session management
    bool startNewSession(const String& deviceName);
    void endCurrentSession();
    bool saveLogToSession(const LogPacket& packet, const String& deviceName);
    String getCurrentSessionFile() const { return currentSessionFile; }

    // File operations
    std::vector<String> loadLogFile(const String& path);
    bool saveLogFile(const String& path, const std::vector<String>& lines);
    bool deleteFile(const String& path);
    bool createDirectory(const String& path);

    // File browsing
    std::vector<FileInfo> listDirectory(const String& path = "/");
    std::vector<FileInfo> listLogFiles();
    FileInfo getFileInfo(const String& path);

    // Storage info
    unsigned long getTotalSpace() const;
    unsigned long getUsedSpace() const;
    unsigned long getFreeSpace() const;

    // Configuration
    void setLogDirectory(const String& dir) { logDirectory = dir; }
    void setMaxFileSize(unsigned long maxSize) { maxFileSize = maxSize; }
    void setMaxFiles(int maxFiles) { maxFilesPerSession = maxFiles; }

   private:
    // Configuration
    int csPin;
    String logDirectory;
    unsigned long maxFileSize;
    int maxFilesPerSession;

    // Current session
    String currentSessionFile;
    String currentDeviceName;
    File currentFile;
    unsigned long currentFileSize;
    int currentFileNumber;

    // Internal methods
    String generateSessionFileName(const String& deviceName);
    String generateLogFileName(const String& deviceName, int fileNumber);
    bool rotateLogFile();
    String formatTimestamp(unsigned long timestamp);
    String formatFileSize(unsigned long bytes);
    bool ensureDirectoryExists(const String& path);
    void closeCurrentFile();
};

}  // namespace Core
}  // namespace BTLogger