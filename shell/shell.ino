#include <kernelOS.h>
 

/*
  Shell Application - Full Featured with Directory Support
*/

// Global text display for kernel debug output
Text* globalDisplayText = nullptr;

// Shell state structure
struct ShellState {
  char currentDir[128];
  int cmdLen;
  Terminal* terminal = nullptr;
};

//pre define function
void processCommand(const char* cmdLine, char* currentDir, Text& t);

void printPrompt(const char* currentDir, Text& textToPrint) {
  // println("hello world");
  // ui.update();
  // displayFrameBuffer();
  // delay(10000);
  // delay(2000);

  println("Printing intial prompt");
#
  textToPrint.print(F("shell:"));
  textToPrint.print(currentDir);
  textToPrint.print(F("$ "));

  ui.update();
  displayFrameBuffer();

//   while(1);
}

void killShell(void* taskStatePtr)
{
    if(taskStatePtr != nullptr)
    {
        uint8_t& taskState = *((uint8_t*)(taskStatePtr));

        taskState = 3;/*Terminated*/
    }else
    {
        println("ERROR line ",__LINE__," in ",__FILE__,", taskStatePtr is a nullptr (can't delete the shellTask when we should be deleting it)");
    }
}

//function that actually works with the screen
void shellTask() {
    ShellState* state = new ShellState();
    strcpy(state->currentDir, "/");
    state->cmdLen = 0;
    state->terminal = &ui.addTerminal("terminal", "middle", "middle", 200, 200, color(0, 0, 0), color(0, 255, 0));

    // Set global display for kernel output
    globalDisplayText = &state->terminal->text;
    KernelOS::setDisplayOutput(globalDisplayText);

    printPrompt(state->currentDir, state->terminal->text);

    // This is now the thread's infinite loop
    while(true) {
        if(state->terminal->cmdAvailable) {
            state->terminal->cmdAvailable = 0;
            processCommand(state->terminal->cmdBuffer, state->currentDir, state->terminal->text);
            printPrompt(state->currentDir, state->terminal->text);
        }

        // Record that this task is still alive
        KernelOS::recordTaskActivity(KernelOS::getCurrentTaskId());

        // IMPORTANT: Don't busy-wait!
        rtos::ThisThread::sleep_for(std::chrono::milliseconds(10));
    }

    // Thread cleanup (if it ever exits)
    delete state;
}

//function to update the ui and the screen
void updateDisplayTask()
{
  while(true) {
    KernelOS::getDisplayMutex().lock();
    ui.update();

    //don't render out to the  if we are waiting of keyboard input
    if(!kbd.Available)
    {
        displayFrameBuffer();//render on the screen
    }
    KernelOS::getDisplayMutex().unlock();

    // Record that this task is still alive
    KernelOS::recordTaskActivity(KernelOS::getCurrentTaskId());

    rtos::ThisThread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
  }
}

void resolvePath(const char* path, const char* currentDir, char* output, size_t outputSize) {
  if (path[0] == '/') {
    // Absolute path
    strncpy(output, path, outputSize - 1);
    output[outputSize - 1] = '\0';
  } else if (strcmp(path, "..") == 0) {
    // Go up one directory
    strncpy(output, currentDir, outputSize - 1);
    output[outputSize - 1] = '\0';
    
    // Remove trailing slash
    size_t len = strlen(output);
    if (len > 1 && output[len - 1] == '/') {
      output[len - 1] = '\0';
      len--;
    }
    
    // Find last slash
    char* lastSlash = strrchr(output, '/');
    if (lastSlash && lastSlash != output) {
      *(lastSlash + 1) = '\0';
    } else {
      strcpy(output, "/");
    }
  } else {
    // Relative path
    strncpy(output, currentDir, outputSize - 1);
    output[outputSize - 1] = '\0';
    
    size_t len = strlen(output);
    if (len > 0 && output[len - 1] != '/') {
      strncat(output, "/", outputSize - len - 1);
    }
    strncat(output, path, outputSize - strlen(output) - 1);
  }

}

void cmdHelp(Text& textToPrint) {
  textToPrint.println(F("\nFile Operations:"));
  textToPrint.println(F("  ls [path]           - List files"));
  textToPrint.println(F("  cd <path>           - Change directory"));
  textToPrint.println(F("  pwd                 - Print working directory"));
  textToPrint.println(F("  cat <file>          - Display file"));
  textToPrint.println(F("  edit <file>         - Edit text file"));
  textToPrint.println(F("  grep <pattern> <file> - Search in file"));
  textToPrint.println(F("  rm <file>           - Remove file"));
  textToPrint.println(F("  mv <src> <dst>      - Move/rename file"));
  textToPrint.println(F("  cp <src> <dst>      - Copy file"));
  textToPrint.println(F("  touch <file>        - Create file"));
  textToPrint.println(F("  mkdir <dir>         - Create directory"));
  textToPrint.println(F("  rmdir <dir>         - Remove directory"));
  textToPrint.println(F("  echo [text]         - Print text"));
  textToPrint.println(F("  echo <text> > <file>- Write to file"));
  
  textToPrint.println(F("\nSystem Operations:"));
  textToPrint.println(F("  ps                  - List tasks"));
  textToPrint.println(F("  meminfo             - Memory info"));
  textToPrint.println(F("  hwinfo              - Hardware Info"));
  textToPrint.println(F("  compact             - Compact memory"));
  textToPrint.println(F("  uptime              - System uptime"));
  textToPrint.println(F("  clear               - Clear screen"));
  textToPrint.println(F("  help                - Show this help\n"));
}

void cmdLs(const char* path, const char* currentDir, Text& textToPrint) {
  char fullPath[128];
  
  if (path[0] == '\0') {
    strncpy(fullPath, currentDir, sizeof(fullPath) - 1);
  } else {
    resolvePath(path, currentDir, fullPath, sizeof(fullPath));
  }
  fullPath[sizeof(fullPath) - 1] = '\0';
  
  int dh = OS::opendir(fullPath);
  if (dh < 0) {
    textToPrint.println(F("Error: Cannot open directory"));
    return;
  }
  
  textToPrint.println();
  
  DirEntry entry;
  while (OS::readdir(dh, &entry)) {
    if (entry.isDirectory) {
      textToPrint.print(F("  [DIR]  "));
      textToPrint.println(entry.name);
    } else {
      textToPrint.print(F("  [FILE] "));
      textToPrint.print(entry.name);
      textToPrint.print(F("\t\t"));
      textToPrint.print(entry.size);
      textToPrint.println(F(" bytes"));
    }
  }
  
  textToPrint.println();
  OS::closedir(dh);
}

void cmdCd(const char* path, char* currentDir, Text& textToPrint) {
  if (path[0] == '\0') {
    strcpy(currentDir, "/");
    return;
  }
  
  char newPath[128];
  resolvePath(path, currentDir, newPath, sizeof(newPath));
  
  int dh = OS::opendir(newPath);
  if (dh < 0) {
    textToPrint.println(F("Error: Directory not found"));
    return;
  }
  
  OS::closedir(dh);
  strcpy(currentDir, newPath);
  
  size_t len = strlen(currentDir);
  if (len > 0 && currentDir[len - 1] != '/') {
    strncat(currentDir, "/", 127 - len);
  }
}

void cmdPwd(const char* currentDir, Text& textToPrint) {
  textToPrint.println(currentDir);
}

void cmdCat(const char* filename, const char* currentDir, Text& textToPrint) {
  if (filename[0] == '\0') {
    textToPrint.println(F("Error: No filename"));
    return;
  }
  
  char filepath[128];
  resolvePath(filename, currentDir, filepath, sizeof(filepath));
  
  int fd = OS::open(filepath, false);
  if (fd < 0) {
    textToPrint.println(F("Error: Cannot open file"));
    return;
  }
  
  textToPrint.println();
  char buffer[128];
  int bytesRead;
  
  while ((bytesRead = OS::read(fd, buffer, sizeof(buffer) - 1)) > 0) {
    buffer[bytesRead] = '\0';
    textToPrint.print(buffer);
  }
  
  textToPrint.println();
  OS::close(fd);
}

void cmdRm(const char* filename, const char* currentDir, Text& textToPrint) {
  if (filename[0] == '\0') {
    textToPrint.println(F("Error: No filename"));
    return;
  }
  
  char filepath[128];
  resolvePath(filename, currentDir, filepath, sizeof(filepath));
  
  if (OS::remove(filepath)) {
    textToPrint.println(F("File removed"));
  } else {
    textToPrint.println(F("Error: Cannot remove file"));
  }
}

void cmdTouch(const char* filename, const char* currentDir, Text& textToPrint) {
  if (filename[0] == '\0') {
    textToPrint.println(F("Error: No filename"));
    return;
  }
  
  char filepath[128];
  resolvePath(filename, currentDir, filepath, sizeof(filepath));
  
  int fd = OS::open(filepath, true);
  if (fd >= 0) {
    OS::close(fd);
    textToPrint.println(F("File created"));
  } else {
    textToPrint.println(F("Error: Cannot create file"));
  }
}

void cmdMkdir(const char* dirname, const char* currentDir, Text& textToPrint) {
  if (dirname[0] == '\0') {
    textToPrint.println(F("Error: No directory name"));
    return;
  }
  
  char dirpath[128];
  resolvePath(dirname, currentDir, dirpath, sizeof(dirpath));
  
  if (OS::mkdir(dirpath)) {
    textToPrint.println(F("Directory created"));
  } else {
    textToPrint.println(F("Error: Cannot create directory"));
  }
}

void cmdRmdir(const char* dirname, const char* currentDir, Text& textToPrint) {
  if (dirname[0] == '\0') {
    textToPrint.println(F("Error: No directory name"));
    return;
  }
  
  char dirpath[128];
  resolvePath(dirname, currentDir, dirpath, sizeof(dirpath));
  
  if (OS::rmdir(dirpath)) {
    textToPrint.println(F("Directory removed"));
  } else {
    textToPrint.println(F("Error: Cannot remove directory (must be empty)"));
  }
}

void cmdEcho(const char* fullCmd, const char* currentDir, Text& textToPrint) {
  const char* redirectPos = strstr(fullCmd, ">");
  
  if (redirectPos) {
    // Echo to file
    char text[128] = {0};
    const char* textStart = fullCmd + 5; // Skip "echo "
    while (*textStart == ' ') textStart++;
    
    size_t textLen = redirectPos - textStart;
    while (textLen > 0 && textStart[textLen - 1] == ' ') textLen--;
    
    if (textLen >= sizeof(text)) textLen = sizeof(text) - 1;
    strncpy(text, textStart, textLen);
    text[textLen] = '\0';
    
    char filename[64] = {0};
    const char* filenameStart = redirectPos + 1;
    while (*filenameStart == ' ') filenameStart++;
    
    strncpy(filename, filenameStart, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';
    
    size_t fnLen = strlen(filename);
    while (fnLen > 0 && filename[fnLen - 1] == ' ') {
      filename[--fnLen] = '\0';
    }
    
    if (filename[0] == '\0') {
      textToPrint.println(F("Error: No filename"));
      return;
    }
    
    char filepath[128];
    resolvePath(filename, currentDir, filepath, sizeof(filepath));
    
    OS::remove(filepath);
    
    int fd = OS::open(filepath, true);
    if (fd >= 0) {
      OS::write(fd, text, strlen(text));
      OS::write(fd, "\n", 1);
      OS::close(fd);
      textToPrint.println(F("Text written to file"));
    } else {
      textToPrint.println(F("Error: Cannot write to file"));
    }
  } else {
    // Echo to terminal
    const char* text = fullCmd + 5; // Skip "echo "
    while (*text == ' ') text++;
    textToPrint.println(text);
  }
}

void cmdCompact(Text& textToPrint) {
  textToPrint.println(F("Compacting memory..."));
  OS::compact();
  textToPrint.println(F("Done"));
}

void cmdUptime(Text& textToPrint) {
  uint32_t up = OS::uptime();
  uint32_t seconds = up / 1000;
  uint32_t minutes = seconds / 60;
  uint32_t hours = minutes / 60;
  
  textToPrint.print(F("Uptime: "));
  textToPrint.print(hours);
  textToPrint.print(F("h "));
  textToPrint.print(minutes % 60);
  textToPrint.print(F("m "));
  textToPrint.print(seconds % 60);
  textToPrint.println(F("s"));
}

void cmdClear(Text& textToPrint) {
  // for (int i = 0; i < 50; i++) {
  //   textToPrint.println();
  // }
  textToPrint.fixedText = ""; //actually clear the fixed text
  textToPrint.x = 0;//reset text position (reset scroll)
  textToPrint.y = 0;//reset text position (reset scroll)
  textToPrint.println(F("================================="));
  textToPrint.println(F("  ChongOS Shell"));
  textToPrint.println(F("=================================\n"));
}

void cmdGrep(const char* fullCmd, const char* currentDir, Text& textToPrint) {
  // Parse: grep <pattern> <file>
  const char* args = fullCmd + 5; // Skip "grep "
  while (*args == ' ') args++;
  
  char pattern[64] = {0};
  char filename[64] = {0};
  
  // Extract pattern (first argument)
  const char* space = strchr(args, ' ');
  if (!space) {
    textToPrint.println(F("Usage: grep <pattern> <file>"));
    return;
  }
  
  size_t patternLen = space - args;
  if (patternLen >= sizeof(pattern)) patternLen = sizeof(pattern) - 1;
  strncpy(pattern, args, patternLen);
  pattern[patternLen] = '\0';
  
  // Extract filename (second argument)
  const char* filenameStart = space + 1;
  while (*filenameStart == ' ') filenameStart++;
  strncpy(filename, filenameStart, sizeof(filename) - 1);
  filename[sizeof(filename) - 1] = '\0';
  
  // Trim trailing spaces
  size_t fnLen = strlen(filename);
  while (fnLen > 0 && filename[fnLen - 1] == ' ') {
    filename[--fnLen] = '\0';
  }
  
  if (filename[0] == '\0') {
    textToPrint.println(F("Usage: grep <pattern> <file>"));
    return;
  }
  
  // Open file
  char filepath[128];
  resolvePath(filename, currentDir, filepath, sizeof(filepath));
  
  int fd = OS::open(filepath, false);
  if (fd < 0) {
    textToPrint.println(F("Error: Cannot open file"));
    return;
  }
  
  // Read file line by line and search for pattern
  char line[128];
  int linePos = 0;
  bool foundMatch = false;
  
  textToPrint.println();
  
  char buffer[128];
  int bytesRead;
  while ((bytesRead = OS::read(fd, buffer, sizeof(buffer) - 1)) > 0) {
    for (int i = 0; i < bytesRead; i++) {
      char c = buffer[i];
      
      if (c == '\n' || c == '\r') {
        if (linePos > 0) {
          line[linePos] = '\0';
          
          // Simple substring search (case-sensitive)
          if (strstr(line, pattern) != nullptr) {
            textToPrint.println(line);
            foundMatch = true;
          }
          
          linePos = 0;
        }
      } else if ((unsigned int)linePos < sizeof(line) - 1) {
        line[linePos++] = c;
      }
    }
  }
  
  // Check last line
  if (linePos > 0) {
    line[linePos] = '\0';
    if (strstr(line, pattern) != nullptr) {
      textToPrint.println(line);
      foundMatch = true;
    }
  }
  
  if (!foundMatch) {
    textToPrint.println(F("No matches found"));
  }
  
  textToPrint.println();
  OS::close(fd);
}

void cmdMv(const char* args, const char* currentDir, Text& textToPrint) {
  // Parse: mv <src> <dst>
  char src[64] = {0};
  char dst[64] = {0};
  
  const char* space = strchr(args, ' ');
  if (!space) {
    textToPrint.println(F("Usage: mv <source> <destination>"));
    return;
  }
  
  // Extract source
  size_t srcLen = space - args;
  if (srcLen >= sizeof(src)) srcLen = sizeof(src) - 1;
  strncpy(src, args, srcLen);
  src[srcLen] = '\0';
  
  // Extract destination
  const char* dstStart = space + 1;
  while (*dstStart == ' ') dstStart++;
  strncpy(dst, dstStart, sizeof(dst) - 1);
  dst[sizeof(dst) - 1] = '\0';
  
  // Trim trailing spaces
  size_t dstLen = strlen(dst);
  while (dstLen > 0 && dst[dstLen - 1] == ' ') {
    dst[--dstLen] = '\0';
  }
  
  if (dst[0] == '\0') {
    textToPrint.println(F("Usage: mv <source> <destination>"));
    return;
  }
  
  // Resolve paths
  char srcPath[128];
  char dstPath[128];
  resolvePath(src, currentDir, srcPath, sizeof(srcPath));
  resolvePath(dst, currentDir, dstPath, sizeof(dstPath));
  
  // Check if source exists
  if (!OS::exists(srcPath)) {
    textToPrint.println(F("Error: Source file not found"));
    return;
  }
  
  // Copy file contents
  int fdSrc = OS::open(srcPath, false);
  if (fdSrc < 0) {
    textToPrint.println(F("Error: Cannot open source file"));
    return;
  }
  
  // Remove destination if it exists
  OS::remove(dstPath);
  
  int fdDst = OS::open(dstPath, true);
  if (fdDst < 0) {
    OS::close(fdSrc);
    textToPrint.println(F("Error: Cannot create destination file"));
    return;
  }
  
  // Copy data
  char buffer[128];
  int bytesRead;
  while ((bytesRead = OS::read(fdSrc, buffer, sizeof(buffer))) > 0) {
    OS::write(fdDst, buffer, bytesRead);
    OS::yield(); // Yield during long operation
  }
  
  OS::close(fdSrc);
  OS::close(fdDst);
  
  // Remove source file
  if (OS::remove(srcPath)) {
    textToPrint.println(F("File moved"));
  } else {
    textToPrint.println(F("Warning: Copy succeeded but could not delete source"));
  }
}

void cmdCp(const char* args, const char* currentDir, Text& textToPrint) {
  // Parse: cp <src> <dst>
  char src[64] = {0};
  char dst[64] = {0};
  
  const char* space = strchr(args, ' ');
  if (!space) {
    textToPrint.println(F("Usage: cp <source> <destination>"));
    return;
  }
  
  // Extract source
  size_t srcLen = space - args;
  if (srcLen >= sizeof(src)) srcLen = sizeof(src) - 1;
  strncpy(src, args, srcLen);
  src[srcLen] = '\0';
  
  // Extract destination
  const char* dstStart = space + 1;
  while (*dstStart == ' ') dstStart++;
  strncpy(dst, dstStart, sizeof(dst) - 1);
  dst[sizeof(dst) - 1] = '\0';
  
  // Trim trailing spaces
  size_t dstLen = strlen(dst);
  while (dstLen > 0 && dst[dstLen - 1] == ' ') {
    dst[--dstLen] = '\0';
  }
  
  if (dst[0] == '\0') {
    textToPrint.println(F("Usage: cp <source> <destination>"));
    return;
  }
  
  // Resolve paths
  char srcPath[128];
  char dstPath[128];
  resolvePath(src, currentDir, srcPath, sizeof(srcPath));
  resolvePath(dst, currentDir, dstPath, sizeof(dstPath));
  
  // Check if source exists
  if (!OS::exists(srcPath)) {
    textToPrint.println(F("Error: Source file not found"));
    return;
  }
  
  // Open source
  int fdSrc = OS::open(srcPath, false);
  if (fdSrc < 0) {
    textToPrint.println(F("Error: Cannot open source file"));
    return;
  }
  
  // Remove destination if it exists
  OS::remove(dstPath);
  
  // Open destination
  int fdDst = OS::open(dstPath, true);
  if (fdDst < 0) {
    OS::close(fdSrc);
    textToPrint.println(F("Error: Cannot create destination file"));
    return;
  }
  
  // Copy data
  char buffer[128];
  int bytesRead;
  while ((bytesRead = OS::read(fdSrc, buffer, sizeof(buffer))) > 0) {
    OS::write(fdDst, buffer, bytesRead);
    OS::yield(); // Yield during long operation
  }
  
  OS::close(fdSrc);
  OS::close(fdDst);
  
  textToPrint.println(F("File copied"));
}

/*
  Ed-style Text Editor
  Add this function with the other cmd* functions in your shell
*/

/*
  Ed-style Text Editor
  Add this function with the other cmd* functions in your shell
*/

void cmdEdit(const char* filename, const char* currentDir, Text& textToPrint) {

  Serial.println("Sry, this won't display on the screen, too much of a spagetti code for me to care to try and make this work");
  textToPrint.println("Sry, this won't display on the screen, too much of a spagetti code for me to care to try and make this work");

  if (filename[0] == '\0') {
    Serial.println(F("Usage: edit <filename>"));
    return;
  }
  
  // Allocate editor buffer on heap instead of stack
  #define MAX_LINES 20
  char (*lines)[128] = (char (*)[128])OS::malloc(MAX_LINES * 128);
  if (!lines) {
    Serial.println(F("Error: Out of memory"));
    return;
  }
  
  int lineCount = 0;
  bool modified = false;
  
  char filepath[128];
  resolvePath(filename, currentDir, filepath, sizeof(filepath));
  
  // Load file
  int fd = OS::open(filepath, false);
  if (fd >= 0) {
    char currentLine[128];
    int linePos = 0;
    char buffer[128];
    int bytesRead;
    
    while ((bytesRead = OS::read(fd, buffer, sizeof(buffer) - 1)) > 0) {
      for (int i = 0; i < bytesRead; i++) {
        char c = buffer[i];
        if (c == '\n' || c == '\r') {
          if (linePos > 0 || c == '\n') {
            if (lineCount >= MAX_LINES) {
              Serial.println(F("Warning: File too large, truncated"));
              break;
            }
            currentLine[linePos] = '\0';
            strcpy(lines[lineCount++], currentLine);
            linePos = 0;
          }
        } else if (linePos < 127) {
          currentLine[linePos++] = c;
        }
      }
      OS::yield();
    }
    
    if (linePos > 0 && lineCount < MAX_LINES) {
      currentLine[linePos] = '\0';
      strcpy(lines[lineCount++], currentLine);
    }
    
    OS::close(fd);
    Serial.print(lineCount);
    Serial.println(F(" lines loaded"));
  } else {
    Serial.println(F("New file"));
  }
  
  Serial.println(F("\nEd-style line editor. Type 'h' for help.\n"));
  
  // Command loop
  char cmd[128];
  while (true) {
    Serial.print(F(": "));
    
    // Read command
    int pos = 0;
    cmd[0] = '\0';
    while (true) {
      while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
          if (pos > 0) {
            Serial.println();
            cmd[pos] = '\0';
            goto process_command;
          }
        } else if (c == 127 || c == 8) {
          if (pos > 0) {
            pos--;
            Serial.write(8);
            Serial.write(' ');
            Serial.write(8);
          }
        } else if (pos < 127) {
          cmd[pos++] = c;
          Serial.write(c);
        }
      }
      OS::yield();
    }
    
process_command:
    if (strlen(cmd) == 0) continue;
    
    char cmdChar = cmd[0];
    
    // Help
    if (strcmp(cmd, "h") == 0) {
      Serial.println(F("\nEditor Commands:"));
      Serial.println(F("  p           - Print all lines"));
      Serial.println(F("  p N         - Print line N"));
      Serial.println(F("  p N,M       - Print lines N to M"));
      Serial.println(F("  a           - Append lines (end with '.')"));
      Serial.println(F("  i N         - Insert before line N (end with '.')"));
      Serial.println(F("  d N         - Delete line N"));
      Serial.println(F("  d N,M       - Delete lines N to M"));
      Serial.println(F("  c N <text>  - Change line N to <text>"));
      Serial.println(F("  w           - Write/save file"));
      Serial.println(F("  q           - Quit (warns if unsaved)"));
      Serial.println(F("  q!          - Quit without saving"));
      Serial.println(F("  h           - Show this help\n"));
      
    // Print
    } else if (strcmp(cmd, "p") == 0) {
      Serial.println();
      for (int i = 0; i < lineCount; i++) {
        Serial.print(i + 1);
        Serial.print(F(": "));
        Serial.println(lines[i]);
      }
      Serial.println();
      
    } else if (cmdChar == 'p' && strlen(cmd) > 1) {
      char* args = cmd + 2;
      while (*args == ' ') args++;
      
      char* comma = strchr(args, ',');
      if (comma) {
        *comma = '\0';
        int start = atoi(args);
        int end = atoi(comma + 1);
        if (start < 1) start = 1;
        if (end > lineCount) end = lineCount;
        Serial.println();
        for (int i = start - 1; i < end; i++) {
          Serial.print(i + 1);
          Serial.print(F(": "));
          Serial.println(lines[i]);
        }
        Serial.println();
      } else {
        int lineNum = atoi(args);
        if (lineNum >= 1 && lineNum <= lineCount) {
          Serial.println();
          Serial.print(lineNum);
          Serial.print(F(": "));
          Serial.println(lines[lineNum - 1]);
          Serial.println();
        }
      }
      
    // Append
    } else if (strcmp(cmd, "a") == 0) {
      Serial.println(F("Append mode (type '.' alone to end):"));
      char line[128];
      while (true) {
        int lpos = 0;
        line[0] = '\0';
        while (true) {
          while (Serial.available() > 0) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') {
              if (lpos > 0) {
                Serial.println();
                line[lpos] = '\0';
                goto append_line;
              }
            } else if (c == 127 || c == 8) {
              if (lpos > 0) {
                lpos--;
                Serial.write(8);
                Serial.write(' ');
                Serial.write(8);
              }
            } else if (lpos < 127) {
              line[lpos++] = c;
              Serial.write(c);
            }
          }
          OS::yield();
        }
append_line:
        if (strcmp(line, ".") == 0) break;
        if (lineCount >= MAX_LINES) {
          Serial.println(F("Error: Editor buffer full"));
          break;
        }
        strcpy(lines[lineCount++], line);
        modified = true;
      }
      
    // Insert
    } else if (cmdChar == 'i' && strlen(cmd) > 1) {
      int beforeLine = atoi(cmd + 2);
      if (beforeLine < 1 || beforeLine > lineCount + 1) {
        Serial.println(F("Error: Invalid line number"));
      } else {
        Serial.println(F("Insert mode (type '.' alone to end):"));
        
        // Allocate temp buffer for new lines
        char (*newLines)[128] = (char (*)[128])OS::malloc(MAX_LINES * 128);
        if (!newLines) {
          Serial.println(F("Error: Out of memory"));
        } else {
          int newCount = 0;
          
          char line[128];
          while (true) {
            int lpos = 0;
            line[0] = '\0';
            while (true) {
              while (Serial.available() > 0) {
                char c = Serial.read();
                if (c == '\n' || c == '\r') {
                  if (lpos > 0) {
                    Serial.println();
                    line[lpos] = '\0';
                    goto insert_line;
                  }
                } else if (c == 127 || c == 8) {
                  if (lpos > 0) {
                    lpos--;
                    Serial.write(8);
                    Serial.write(' ');
                    Serial.write(8);
                  }
                } else if (lpos < 127) {
                  line[lpos++] = c;
                  Serial.write(c);
                }
              }
              OS::yield();
            }
insert_line:
            if (strcmp(line, ".") == 0) break;
            if (newCount >= MAX_LINES) {
              Serial.println(F("Error: Too many lines"));
              break;
            }
            strcpy(newLines[newCount++], line);
          }
          
          if (newCount > 0 && lineCount + newCount <= MAX_LINES) {
            for (int i = lineCount - 1; i >= beforeLine - 1; i--) {
              strcpy(lines[i + newCount], lines[i]);
            }
            for (int i = 0; i < newCount; i++) {
              strcpy(lines[beforeLine - 1 + i], newLines[i]);
            }
            lineCount += newCount;
            modified = true;
          }
          
          OS::free(newLines);
        }
      }
      
    // Delete
    } else if (cmdChar == 'd' && strlen(cmd) > 1) {
      char* args = cmd + 2;
      while (*args == ' ') args++;
      
      char* comma = strchr(args, ',');
      int start, end;
      if (comma) {
        *comma = '\0';
        start = atoi(args);
        end = atoi(comma + 1);
      } else {
        start = end = atoi(args);
      }
      
      if (start < 1 || start > lineCount) {
        Serial.println(F("Error: Invalid line number"));
      } else {
        if (end < start) end = start;
        if (end > lineCount) end = lineCount;
        
        int deleteCount = end - start + 1;
        for (int i = end; i < lineCount; i++) {
          strcpy(lines[i - deleteCount], lines[i]);
        }
        lineCount -= deleteCount;
        modified = true;
        
        Serial.print(deleteCount);
        Serial.println(F(" line(s) deleted"));
      }
      
    // Change
    } else if (cmdChar == 'c' && strlen(cmd) > 1) {
      char* args = cmd + 2;
      while (*args == ' ') args++;
      
      char* space = strchr(args, ' ');
      if (space) {
        *space = '\0';
        int lineNum = atoi(args);
        if (lineNum >= 1 && lineNum <= lineCount) {
          strncpy(lines[lineNum - 1], space + 1, 127);
          lines[lineNum - 1][127] = '\0';
          modified = true;
          Serial.println(F("Line changed"));
        } else {
          Serial.println(F("Error: Invalid line number"));
        }
      } else {
        Serial.println(F("Usage: c <line> <text>"));
      }
      
    // Write
    } else if (strcmp(cmd, "w") == 0) {
      OS::remove(filepath);
      fd = OS::open(filepath, true);
      if (fd >= 0) {
        for (int i = 0; i < lineCount; i++) {
          OS::write(fd, lines[i], strlen(lines[i]));
          OS::write(fd, "\n", 1);
          OS::yield();
        }
        OS::close(fd);
        modified = false;
        Serial.print(lineCount);
        Serial.println(F(" lines written"));
      } else {
        Serial.println(F("Error: Cannot write file"));
      }
      
    // Quit
    } else if (strcmp(cmd, "q") == 0) {
      if (modified) {
        Serial.println(F("Warning: Unsaved changes! Use 'q!' to quit without saving"));
      } else {
        break;
      }
      
    } else if (strcmp(cmd, "q!") == 0) {
      break;
      
    } else {
      Serial.println(F("Unknown command. Type 'h' for help"));
    }
    
    OS::yield();
  }
  
  OS::free(lines);
  Serial.println(F("Editor closed"));
}

void cmdHwinfo(Text& textToPrint) {
  textToPrint.println(F("\n=== Hardware Info ==="));
  
  #ifdef ARDUINO_GIGA
    textToPrint.println(F("Board: Arduino Giga R1 WiFi"));
    textToPrint.println(F("MCU: STM32H747XI (Cortex-M7 + M4)"));
    textToPrint.println(F("RAM: 1 MB"));
    textToPrint.println(F("Flash: 2 MB"));
  #elif defined(ARDUINO_ARCH_RP2040)
    textToPrint.println(F("Board: Raspberry Pi Pico"));
    textToPrint.println(F("MCU: RP2040 (Dual Cortex-M0+)"));
    textToPrint.println(F("RAM: 264 KB"));
    textToPrint.println(F("Flash: 2 MB"));
  #elif defined(ARDUINO_AVR_MEGA2560)
    textToPrint.println(F("Board: Arduino Mega 2560"));
    textToPrint.println(F("MCU: ATmega2560"));
    textToPrint.println(F("RAM: 8 KB"));
    textToPrint.println(F("Flash: 256 KB"));
  #else
    textToPrint.println(F("Board: Unknown/Generic Arduino"));
  #endif
  
  textToPrint.print(F("Clock: "));
  textToPrint.print((uint32_t)SystemCoreClock / 1000000);
  textToPrint.println(F(" MHz"));
  
  textToPrint.print(F("Kernel heap: "));
  textToPrint.print(KERNEL_HEAP_SIZE);
  textToPrint.println(F(" bytes"));
  
  textToPrint.print(F("SD CS Pin: "));
  textToPrint.println(SD_CS_PIN);
  
  textToPrint.println();
}

void processCommand(const char* cmdLine, char* currentDir, Text& t) {
  // Skip leading whitespace
  while (*cmdLine == ' ') cmdLine++;
  if (*cmdLine == '\0') return;
  
  // Parse command and arguments
  char cmd[64] = {0};
  char args[192] = {0};
  
  const char* space = strchr(cmdLine, ' ');
  if (space) {
    size_t cmdLen = space - cmdLine;
    if (cmdLen >= sizeof(cmd)) cmdLen = sizeof(cmd) - 1;
    strncpy(cmd, cmdLine, cmdLen);
    cmd[cmdLen] = '\0';
    
    // Copy args, skipping whitespace
    space++;
    while (*space == ' ') space++;
    strncpy(args, space, sizeof(args) - 1);
    args[sizeof(args) - 1] = '\0';
  } else {
    strncpy(cmd, cmdLine, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';
  }
  
  // Execute commands
  if (strcmp(cmd, "help") == 0) {
    cmdHelp(t);
  } else if (strcmp(cmd, "ls") == 0) {
    cmdLs(args, currentDir, t);
  } else if (strcmp(cmd, "cd") == 0) {
    cmdCd(args, currentDir, t);
  } else if (strcmp(cmd, "pwd") == 0) {
    cmdPwd(currentDir, t);
  } else if (strcmp(cmd, "cat") == 0) {
    cmdCat(args, currentDir, t);
  } else if (strcmp(cmd, "rm") == 0) {
    cmdRm(args, currentDir, t);
  } else if (strcmp(cmd, "touch") == 0) {
    cmdTouch(args, currentDir, t);
  } else if (strcmp(cmd, "mkdir") == 0) {
    cmdMkdir(args, currentDir, t);
  } else if (strcmp(cmd, "rmdir") == 0) {
    cmdRmdir(args, currentDir, t);
  } else if (strcmp(cmd, "echo") == 0) {
    cmdEcho(cmdLine, currentDir, t);
  } else if (strcmp(cmd, "grep") == 0) {
    cmdGrep(cmdLine, currentDir, t);
  } else if (strcmp(cmd, "mv") == 0) {
    cmdMv(args, currentDir, t);
  } else if (strcmp(cmd, "cp") == 0) {
    cmdCp(args, currentDir, t);
  } else if (strcmp(cmd, "ps") == 0) {
    KernelOS::printTaskList(t);
  } else if (strcmp(cmd, "meminfo") == 0) {
    KernelOS::printMemoryInfo(t);
  } else if (strcmp(cmd, "compact") == 0) {
    cmdCompact(t);
  } else if (strcmp(cmd, "uptime") == 0) {
    cmdUptime(t);
  } else if (strcmp(cmd, "clear") == 0) {
    cmdClear(t);
  } else if (strcmp(cmd, "edit") == 0) {
    cmdEdit(args, currentDir, t);
  }  else if (strcmp(cmd, "hwinfo") == 0) {
    cmdHwinfo(t);
  } else {
    t.print(F("Unknown command: "));
    t.println(cmd);
    t.println(F("Type 'help' for available commands"));
  }
}

// ============================================================================
// MONITOR TASK - Auto-recovery and health checking
// ============================================================================

void monitorTask() {
  // Monitor task watches all other tasks for crashes/hangs
  // If a task becomes unresponsive, it attempts recovery
  // After 3 failed recovery attempts, triggers full system reboot
  
  while(true) {
    // Let kernel do health checking
    KernelOS::monitorTaskHealth();
    
    // Record that monitor is alive
    KernelOS::recordTaskActivity(KernelOS::getCurrentTaskId());
    
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(500));
  }
}

// ============================================================================
// MAIN PROGRAM
// ============================================================================

void setup() {
  Wire.begin();
  // Initialize watchdog
  Watchdog &watchdog = Watchdog::get_instance();
  watchdog.start(5000);  // 5 second timeout
  
  // Initialize kernel
  if (!KernelOS::init()) {
    Serial.println(F("FATAL: Kernel init failed"));
    while(1);
  }
  
  // Create monitor task (watches for crashes and auto-recovers)
  int monitorTaskId = KernelOS::createTask("monitor", monitorTask);
  
  // Create shell task
  int shellTaskId = KernelOS::createTask("shell", shellTask);
  
  // Create screen update task
  int updateDisplayTaskId = KernelOS::createTask("display", updateDisplayTask);
  
  if (shellTaskId < 0) {
    KernelOS::panic("Failed to create shell task");
  }
  
  Serial.println(F("Shell ready. Type 'help' for commands."));
  Serial.println(F("Auto-recovery monitor active.\n"));
}

void loop() {
  // Let Mbed's scheduler run automatically
  rtos::ThisThread::sleep_for(osWaitForever);
}

// ============================================================================
// EXAMPLE USAGE
// ============================================================================
/*
Try these commands:

  mkdir /mydir
  cd /mydir
  touch hello.txt
  echo Hello World > hello.txt
  cat hello.txt
  ls
  cd ..
  ls
  rm /mydir/hello.txt
  rmdir /mydir
  
  ps         - See all tasks
  meminfo    - Check memory usage
  compact    - Compact memory if fragmented
  uptime     - System uptime
*/
