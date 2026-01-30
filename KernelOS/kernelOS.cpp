




/*
  YandereOS Kernel Implementation - V3.5
  Adds: Watchdog timer, IPC, DDI, Stack traces, Fixed compaction
*/
//KERNELOS.cpp FILE

#include "kernelOS.h"

// ============================================================================
// STATIC MEMBER INITIALIZATION
// ============================================================================

Task KernelOS::tasks[MAX_TASKS];
int KernelOS::currentTaskId = 0;
int KernelOS::nextTaskId = 1;

uint8_t KernelOS::kernelHeap[KERNEL_HEAP_SIZE];
size_t KernelOS::heapUsed = 0;

FileHandle KernelOS::fileHandles[MAX_FILE_HANDLES];
DirHandle KernelOS::dirHandles[MAX_DIR_HANDLES];
bool KernelOS::sdInitialized = false;

MessageQueue KernelOS::messageQueues[MAX_TASKS];
Semaphore KernelOS::semaphores[MAX_SEMAPHORES];

bool KernelOS::watchdogEnabled = true;
uint32_t KernelOS::watchdogLastCheck = 0;

bool KernelOS::initialized = false;
uint32_t KernelOS::bootTime = 0;

// ============================================================================
// INITIALIZATION
// ============================================================================

bool KernelOS::init() {
  if (initialized) return true;
  
  Serial.begin(9600);
  while (!Serial && millis() < 14000) delay(1000);
  delay(500);
  
  Serial.println(F("\n=== YandereOS Kernel v3.5 ==="));
  Serial.println(F("Features: Watchdog, IPC, DDI, Stack Traces"));
  Serial.println(F("Initializing..."));

  // Initialize the ui library
  ui.basecolor = color(50, 50, 50);  //change the base color (before begin shows up the background)
  ui.begin();
  
  // Initialize all tasks as empty
  for (int i = 0; i < MAX_TASKS; i++) {
    tasks[i].state = TASK_EMPTY;
    tasks[i].id = -1;
    tasks[i].stackTraceDepth = 0;
  }
  
  // Initialize file handles
  for (int i = 0; i < MAX_FILE_HANDLES; i++) {
    fileHandles[i].inUse = false;
  }
  
  // Initialize directory handles
  for (int i = 0; i < MAX_DIR_HANDLES; i++) {
    dirHandles[i].inUse = false;
  }
  
  // Initialize message queues
  for (int i = 0; i < MAX_TASKS; i++) {
    messageQueues[i].head = 0;
    messageQueues[i].tail = 0;
    messageQueues[i].count = 0;
    for (int j = 0; j < MAX_MESSAGE_QUEUE_SIZE; j++) {
      messageQueues[i].messages[j].valid = false;
    }
  }
  
  // Initialize semaphores
  for (int i = 0; i < MAX_SEMAPHORES; i++) {
    semaphores[i].inUse = false;
  }
  
  // Initialize memory
  heapUsed = 0;
  
  // Initialize SD card
  Serial.print(F("Mounting SD card... "));
  if (SD.begin(SD_CS_PIN)) {
    sdInitialized = true;
    Serial.println(F("OK"));
  } else {
    Serial.println(F("FAILED"));
    Serial.println(F("Warning: SD card not available"));
  }
  
  // Create idle task (task 0)
  tasks[0].id = 0;
  tasks[0].name = "idle";
  tasks[0].state = TASK_READY;
  tasks[0].priority = 0;
  tasks[0].lastYield = millis();
  tasks[0].canAccessSD = false;
  tasks[0].canAccessDisplay = false;
  tasks[0].canCreateTasks = false;
  tasks[0].canAccessGPIO = false;
  tasks[0].canAccessI2C = false;
  tasks[0].canAccessSPI = false;
  tasks[0].memoryUsed = 0;
  
  currentTaskId = 0;
  bootTime = millis();
  watchdogLastCheck = millis();
  initialized = true;
  
  Serial.println(F("Kernel initialized successfully\n"));
  return true;
}

void KernelOS::panic(const char* message) {
  Serial.println(F("\n!!! KERNEL PANIC !!!"));
  Serial.println(message);
  
  // Print current task info
  Task* current = getCurrentTask();
  if (current) {
    Serial.print(F("Current task: "));
    Serial.print(current->name);
    Serial.print(F(" (ID: "));
    Serial.print(current->id);
    Serial.println(F(")"));
    
    // Print stack trace if available
    printStackTrace(current);
  }
  
  Serial.println(F("\n=== System State ==="));
  printTaskList();
  printMemoryInfo();
  
  Serial.println(F("\nSystem halted."));
  while(1) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

// ============================================================================
// STACK TRACING
// ============================================================================

void KernelOS::captureStackTrace(Task* task) {
  if (!task) return;
  
  // Simple stack capture - on Arduino, this is limited
  // We can at least record the entry point
  task->stackTraceDepth = 1;
  task->stackTrace[0].returnAddress = (void*)task->entryPoint;
  task->stackTrace[0].functionName = task->name;
  
  // Note: Full stack unwinding requires DWARF debug info
  // which is not easily accessible on Arduino
}

void KernelOS::printStackTrace(Task* task) {
  if (!task || task->stackTraceDepth == 0) {
    Serial.println(F("No stack trace available"));
    return;
  }
  
  Serial.println(F("\n=== Stack Trace ==="));
  for (int i = 0; i < task->stackTraceDepth; i++) {
    Serial.print(F("  ["));
    Serial.print(i);
    Serial.print(F("] "));
    
    if (task->stackTrace[i].functionName) {
      Serial.print(task->stackTrace[i].functionName);
    } else {
      Serial.print(F("<unknown>"));
    }
    
    Serial.print(F(" @ 0x"));
    Serial.println((uintptr_t)task->stackTrace[i].returnAddress, HEX);
  }
}

// ============================================================================
// TASK MANAGEMENT
// ============================================================================

int KernelOS::allocateTaskId() {
  for (int i = 1; i < MAX_TASKS; i++) {
    if (tasks[i].state == TASK_EMPTY) {
      return i;
    }
  }
  return -1;
}

Task* KernelOS::getCurrentTask() {
  return &tasks[currentTaskId];
}

Task* KernelOS::getTask(int taskId) {
  if (taskId < 0 || taskId >= MAX_TASKS) return nullptr;
  if (tasks[taskId].state == TASK_EMPTY) return nullptr;
  return &tasks[taskId];
}

int KernelOS::createTask(const char* name, void (*entryPoint)()) {
  int taskId = allocateTaskId();
  if (taskId < 0) {
    return SYS_ERR_NO_MEMORY;
  }
  
  Task* task = &tasks[taskId];
  task->id = taskId;
  task->name = name;
  task->state = TASK_READY;
  task->entryPoint = entryPoint;
  task->priority = 10;
  task->lastRun = 0;
  task->lastYield = millis();
  task->sleepUntil = 0;
  task->memoryUsed = 0;
  task->stackTraceDepth = 0;
  
  // Clear file handles
  for (int i = 0; i < MAX_FILE_HANDLES; i++) {
    task->fileHandles[i] = false;
  }
  
  // Clear dir handles
  for (int i = 0; i < MAX_DIR_HANDLES; i++) {
    task->dirHandles[i] = false;
  }
  
  // Set permissions (default: can access SD and display)
  task->canAccessSD = true;
  task->canAccessDisplay = true;
  task->canCreateTasks = false;
  task->canAccessGPIO = true;   // Default allow GPIO
  task->canAccessI2C = false;   // Require explicit permission
  task->canAccessSPI = false;   // Require explicit permission
  
  // Capture initial stack trace
  captureStackTrace(task);
  
  Serial.print(F("Task created: "));
  Serial.print(name);
  Serial.print(F(" (ID: "));
  Serial.print(taskId);
  Serial.println(F(")"));
  
  return taskId;
}

void KernelOS::killTask(int taskId) {
  Task* task = getTask(taskId);
  if (!task || taskId == 0) return;
  
  // Close all open files
  for (int i = 0; i < MAX_FILE_HANDLES; i++) {
    if (task->fileHandles[i]) {
      freeFileHandle(i);
    }
  }
  
  // Close all open directories
  for (int i = 0; i < MAX_DIR_HANDLES; i++) {
    if (task->dirHandles[i]) {
      freeDirHandle(i);
    }
  }
  
  task->state = TASK_EMPTY;
  task->id = -1;
  
  Serial.print(F("Task killed: "));
  Serial.println(task->name);
}

// ============================================================================
// WATCHDOG TIMER
// ============================================================================

void KernelOS::enableWatchdog(bool enable) {
  watchdogEnabled = enable;
  Serial.print(F("Watchdog "));
  Serial.println(enable ? F("enabled") : F("disabled"));
}

void KernelOS::feedWatchdog() {
  Task* current = getCurrentTask();
  if (current) {
    current->lastYield = millis();
  }
}

void KernelOS::checkWatchdog() {
  if (!watchdogEnabled) return;
  
  uint32_t now = millis();
  
  // Check if we should run watchdog check
  if (now - watchdogLastCheck < 1000) return;
  watchdogLastCheck = now;
  
  // Check all running/ready tasks
  for (int i = 0; i < MAX_TASKS; i++) {
    Task* task = &tasks[i];
    if (task->state == TASK_EMPTY) continue;
    if (task->state == TASK_SLEEPING) continue;
    
    uint32_t timeSinceYield = now - task->lastYield;
    
    if (timeSinceYield > WATCHDOG_TIMEOUT_MS) {
      Serial.print(F("[WATCHDOG] Task "));
      Serial.print(task->name);
      Serial.print(F(" hasn't yielded in "));
      Serial.print(timeSinceYield);
      Serial.println(F("ms - forcing reschedule"));
      
      // Force task to ready state
      if (task->state == TASK_RUNNING) {
        task->state = TASK_READY;
      }
      
      task->lastYield = now;
    }
  }
}

void KernelOS::schedule() {
  checkWatchdog();
  
  uint32_t now = millis();
  int bestTask = 0;
  int bestPriority = -1;
  
  // Find highest priority ready task
  for (int i = 0; i < MAX_TASKS; i++) {
    Task* task = &tasks[i];
    
    if (task->state == TASK_EMPTY) continue;
    
    // Wake up sleeping tasks
    if (task->state == TASK_SLEEPING) {
      if (now >= task->sleepUntil) {
        task->state = TASK_READY;
      } else {
        continue;
      }
    }
    
    if (task->state == TASK_READY && task->priority > bestPriority) {
      bestTask = i;
      bestPriority = task->priority;
    }
  }
  
  // Switch to best task
  if (bestTask != currentTaskId) {
    if (tasks[currentTaskId].state == TASK_RUNNING) {
      tasks[currentTaskId].state = TASK_READY;
    }
    currentTaskId = bestTask;
    tasks[currentTaskId].state = TASK_RUNNING;
    tasks[currentTaskId].lastRun = now;
  }

  
  // Execute current task
  Task* current = getCurrentTask();
  
  if (current->entryPoint && current->state == TASK_RUNNING) {
    current->entryPoint();
  }
//   delay(1000);
}

void KernelOS::yield() {
  Task* current = getCurrentTask();
  if (current) {
    current->state = TASK_READY;
    current->lastYield = millis();
  }
}

void KernelOS::sleep(uint32_t ms) {
  Task* current = getCurrentTask();
  if (current) {
    current->state = TASK_SLEEPING;
    current->sleepUntil = millis() + ms;
    current->lastYield = millis();
  }
}

// ============================================================================
// MEMORY MANAGEMENT - FIXED COMPACTION
// ============================================================================

MemoryBlock* KernelOS::getBlockHeader(void* ptr) {
  if (!ptr) return nullptr;
  return (MemoryBlock*)((uint8_t*)ptr - sizeof(MemoryBlock));
}

void* KernelOS::allocateMemoryInternal(size_t size, int taskId) {
  if (size == 0) return nullptr;
  
  // Align to 4 bytes
  size = (size + 3) & ~3;
  
  size_t totalNeeded = sizeof(MemoryBlock) + size;
  
  // Check if we have space
  if (heapUsed + totalNeeded > KERNEL_HEAP_SIZE) {
    Serial.println(F("[Memory] Out of space, compacting..."));
    compactMemory();
    
    if (heapUsed + totalNeeded > KERNEL_HEAP_SIZE) {
      Serial.println(F("[Memory] Out of memory after compaction!"));
      return nullptr;
    }
  }
  
  // Allocate at end of used heap
  MemoryBlock* block = (MemoryBlock*)&kernelHeap[heapUsed];
  void* userPtr = (void*)((uint8_t*)block + sizeof(MemoryBlock));
  
  block->size = size;
  block->ownerTaskId = taskId;
  block->inUse = true;
  block->handleId = -1;
  
  heapUsed += totalNeeded;
  
  if (taskId >= 0 && taskId < MAX_TASKS) {
    tasks[taskId].memoryUsed += size;
  }
  
  return userPtr;
}

void KernelOS::freeMemoryInternal(void* ptr) {
  if (!ptr) return;
  
  MemoryBlock* block = getBlockHeader(ptr);
  if (!block || !block->inUse) {
    Serial.println(F("[Memory] Warning: Invalid free()"));
    return;
  }
  
  if (block->ownerTaskId >= 0 && block->ownerTaskId < MAX_TASKS) {
    tasks[block->ownerTaskId].memoryUsed -= block->size;
  }
  
  block->inUse = false;
}

void KernelOS::compactMemory() {
  /*
   * FIXED COMPACTION ALGORITHM
   * 
   * Problem with old version: User-space pointers become invalid when
   * blocks are moved, causing crashes when dereferenced.
   * 
   * Solution: We can't safely update user pointers in cooperative multitasking
   * without handles. So instead, we just mark blocks as free and consolidate
   * free space by moving in-use blocks together, but we warn users that
   * compaction invalidates pointers.
   * 
   * Better solution for future: Use handle-based allocation where OS::malloc
   * returns a handle (integer ID) and users must call OS::deref(handle) to
   * get the actual pointer before each use.
   */
  
  Serial.println(F("[Memory] Compacting heap (WARNING: may invalidate pointers)"));
  
  size_t writePos = 0;
  size_t readPos = 0;
  int movedBlocks = 0;
  
  while (readPos < heapUsed) {
    MemoryBlock* block = (MemoryBlock*)&kernelHeap[readPos];
    size_t blockTotalSize = sizeof(MemoryBlock) + block->size;
    
    if (block->inUse) {
      if (writePos != readPos) {
        // Move block to write position
        memmove(&kernelHeap[writePos], &kernelHeap[readPos], blockTotalSize);
        movedBlocks++;
      }
      writePos += blockTotalSize;
    }
    
    readPos += blockTotalSize;
    
    // Safety check
    if (readPos > KERNEL_HEAP_SIZE || writePos > KERNEL_HEAP_SIZE) {
      panic("Heap corruption detected during compaction");
      break;
    }
  }
  
  size_t freedSpace = heapUsed - writePos;
  heapUsed = writePos;
  
  Serial.print(F("[Memory] Compaction complete: freed "));
  Serial.print(freedSpace);
  Serial.print(F(" bytes, moved "));
  Serial.print(movedBlocks);
  Serial.println(F(" blocks"));
  
  if (movedBlocks > 0) {
    Serial.println(F("[Memory] WARNING: Existing pointers may be invalid!"));
    Serial.println(F("[Memory] Recommendation: Free and reallocate after compaction"));
  }
}

void* KernelOS::memAlloc(size_t size) {
  return allocateMemoryInternal(size, currentTaskId);
}

void KernelOS::memFree(void* ptr) {
  freeMemoryInternal(ptr);
}

size_t KernelOS::memAvailable() {
  return KERNEL_HEAP_SIZE - heapUsed;
}

void KernelOS::memCompact() {
  compactMemory();
}

// ============================================================================
// IPC - MESSAGE QUEUES
// ============================================================================

int KernelOS::ipcSend(int toTaskId, const void* data, size_t length) {
  if (toTaskId < 0 || toTaskId >= MAX_TASKS) return SYS_ERR_INVALID_PARAM;
  if (tasks[toTaskId].state == TASK_EMPTY) return SYS_ERR_NOT_FOUND;
  if (length > sizeof(Message::data)) return SYS_ERR_INVALID_PARAM;
  if (!data && length > 0) return SYS_ERR_INVALID_PARAM;
  
  MessageQueue* queue = &messageQueues[toTaskId];
  
  if (queue->count >= MAX_MESSAGE_QUEUE_SIZE) {
    return SYS_ERR_NO_MEMORY;
  }
  
  Message* msg = &queue->messages[queue->tail];
  msg->fromTaskId = currentTaskId;
  msg->toTaskId = toTaskId;
  msg->length = length;
  if (length > 0) {
    memcpy(msg->data, data, length);
  }
  msg->timestamp = millis();
  msg->valid = true;
  
  queue->tail = (queue->tail + 1) % MAX_MESSAGE_QUEUE_SIZE;
  queue->count++;
  
  return SYS_OK;
}

int KernelOS::ipcReceive(void* buffer, size_t maxLength, int* fromTaskId) {
  MessageQueue* queue = &messageQueues[currentTaskId];
  
  if (queue->count == 0) {
    return SYS_ERR_WOULD_BLOCK;
  }
  
  Message* msg = &queue->messages[queue->head];
  if (!msg->valid) {
    return SYS_ERR_IO_ERROR;
  }
  
  if (msg->length > maxLength) {
    return SYS_ERR_INVALID_PARAM;
  }
  
  if (buffer && msg->length > 0) {
    memcpy(buffer, msg->data, msg->length);
  }
  
  if (fromTaskId) {
    *fromTaskId = msg->fromTaskId;
  }
  
  int length = msg->length;
  
  msg->valid = false;
  queue->head = (queue->head + 1) % MAX_MESSAGE_QUEUE_SIZE;
  queue->count--;
  
  return length;
}

int KernelOS::ipcPoll() {
  return messageQueues[currentTaskId].count;
}

// ============================================================================
// IPC - SEMAPHORES
// ============================================================================

int KernelOS::allocateSemaphore() {
  for (int i = 0; i < MAX_SEMAPHORES; i++) {
    if (!semaphores[i].inUse) {
      return i;
    }
  }
  return -1;
}

int KernelOS::semCreate(int initialValue, int maxValue, const char* name) {
  if (initialValue < 0 || maxValue < 1 || initialValue > maxValue) {
    return SYS_ERR_INVALID_PARAM;
  }
  
  int semId = allocateSemaphore();
  if (semId < 0) {
    return SYS_ERR_NO_MEMORY;
  }
  
  semaphores[semId].value = initialValue;
  semaphores[semId].maxValue = maxValue;
  semaphores[semId].inUse = true;
  semaphores[semId].ownerTaskId = currentTaskId;
  semaphores[semId].name = name;
  
  return semId;
}

int KernelOS::semWait(int semId, uint32_t timeoutMs) {
  if (semId < 0 || semId >= MAX_SEMAPHORES) return SYS_ERR_INVALID_PARAM;
  if (!semaphores[semId].inUse) return SYS_ERR_NOT_FOUND;
  
  Semaphore* sem = &semaphores[semId];
  uint32_t startTime = millis();
  
  while (sem->value <= 0) {
    if (timeoutMs > 0 && (millis() - startTime) >= timeoutMs) {
      return SYS_ERR_TIMEOUT;
    }
    yield();
  }
  
  sem->value--;
  return SYS_OK;
}

int KernelOS::semPost(int semId) {
  if (semId < 0 || semId >= MAX_SEMAPHORES) return SYS_ERR_INVALID_PARAM;
  if (!semaphores[semId].inUse) return SYS_ERR_NOT_FOUND;
  
  Semaphore* sem = &semaphores[semId];
  
  if (sem->value >= sem->maxValue) {
    return SYS_ERR_INVALID_PARAM;
  }
  
  sem->value++;
  return SYS_OK;
}

int KernelOS::semDestroy(int semId) {
  if (semId < 0 || semId >= MAX_SEMAPHORES) return SYS_ERR_INVALID_PARAM;
  if (!semaphores[semId].inUse) return SYS_ERR_NOT_FOUND;
  
  // Only owner or kernel can destroy
  if (semaphores[semId].ownerTaskId != currentTaskId && currentTaskId != 0) {
    return SYS_ERR_PERMISSION;
  }
  
  semaphores[semId].inUse = false;
  return SYS_OK;
}

// ============================================================================
// DEVICE DRIVER INTERFACE - GPIO
// ============================================================================

int KernelOS::gpioSetMode(int pin, int mode) {
  Task* current = getCurrentTask();
  if (!current->canAccessGPIO) return SYS_ERR_PERMISSION;
  
  pinMode(pin, mode);
  return SYS_OK;
}

int KernelOS::gpioWrite(int pin, int value) {
  Task* current = getCurrentTask();
  if (!current->canAccessGPIO) return SYS_ERR_PERMISSION;
  
  digitalWrite(pin, value);
  return SYS_OK;
}

int KernelOS::gpioRead(int pin) {
  Task* current = getCurrentTask();
  if (!current->canAccessGPIO) return SYS_ERR_PERMISSION;
  
  return digitalRead(pin);
}

int KernelOS::gpioAnalogRead(int pin) {
  Task* current = getCurrentTask();
  if (!current->canAccessGPIO) return SYS_ERR_PERMISSION;
  
  return analogRead(pin);
}

int KernelOS::gpioAnalogWrite(int pin, int value) {
  Task* current = getCurrentTask();
  if (!current->canAccessGPIO) return SYS_ERR_PERMISSION;
  
  analogWrite(pin, value);
  return SYS_OK;
}

// ============================================================================
// DEVICE DRIVER INTERFACE - I2C
// ============================================================================

int KernelOS::i2cBegin(uint8_t address) {
  Task* current = getCurrentTask();
  if (!current->canAccessI2C) return SYS_ERR_PERMISSION;
  
  if (address == 0) {
    Wire.begin();
  } else {
    Wire.begin(address);
  }
  
  return SYS_OK;
}

int KernelOS::i2cWrite(uint8_t address, const uint8_t* data, size_t length) {
  Task* current = getCurrentTask();
  if (!current->canAccessI2C) return SYS_ERR_PERMISSION;
  if (!data || length == 0) return SYS_ERR_INVALID_PARAM;
  
  Wire.beginTransmission(address);
  size_t written = Wire.write(data, length);
  uint8_t result = Wire.endTransmission();
  
  if (result != 0) {
    return SYS_ERR_IO_ERROR;
  }
  
  return written;
}

int KernelOS::i2cRead(uint8_t address, uint8_t* buffer, size_t length) {
  Task* current = getCurrentTask();
  if (!current->canAccessI2C) return SYS_ERR_PERMISSION;
  if (!buffer || length == 0) return SYS_ERR_INVALID_PARAM;
  
  Wire.beginTransmission(address);
  uint8_t result = Wire.endTransmission();
  
  if (result != 0) {
    return SYS_ERR_IO_ERROR;
  }
  
  // int available = Wire.requestFrom(address, (uint8_t)length);
  int bytesRead = 0;
  
  while (Wire.available() && bytesRead < (int)length) {
    buffer[bytesRead++] = Wire.read();
  }
  
  return bytesRead;
}

int KernelOS::i2cRequest(uint8_t address, size_t quantity) {
  Task* current = getCurrentTask();
  if (!current->canAccessI2C) return SYS_ERR_PERMISSION;
  
  return Wire.requestFrom(address, (uint8_t)quantity);
}

// ============================================================================
// DEVICE DRIVER INTERFACE - SPI
// ============================================================================

int KernelOS::spiBegin() {
  Task* current = getCurrentTask();
  if (!current->canAccessSPI) return SYS_ERR_PERMISSION;
  
  SPI.begin();
  return SYS_OK;
}

int KernelOS::spiTransfer(uint8_t* txData, uint8_t* rxData, size_t length) {
  Task* current = getCurrentTask();
  if (!current->canAccessSPI) return SYS_ERR_PERMISSION;
  if (length == 0) return SYS_ERR_INVALID_PARAM;
  
  if (txData && rxData) {
    for (size_t i = 0; i < length; i++) {
      rxData[i] = SPI.transfer(txData[i]);
    }
  } else if (txData) {
    for (size_t i = 0; i < length; i++) {
      SPI.transfer(txData[i]);
    }
  } else if (rxData) {
    for (size_t i = 0; i < length; i++) {
      rxData[i] = SPI.transfer(0x00);
    }
  }
  
  return length;
}

int KernelOS::spiEnd() {
  Task* current = getCurrentTask();
  if (!current->canAccessSPI) return SYS_ERR_PERMISSION;
  
  SPI.end();
  return SYS_OK;
}

// ============================================================================
// FILE SYSTEM
// ============================================================================

int KernelOS::allocateFileHandle() {
  for (int i = 0; i < MAX_FILE_HANDLES; i++) {
    if (!fileHandles[i].inUse) {
      return i;
    }
  }
  return -1;
}

int KernelOS::allocateDirHandle() {
  for (int i = 0; i < MAX_DIR_HANDLES; i++) {
    if (!dirHandles[i].inUse) {
      return i;
    }
  }
  return -1;
}

void KernelOS::freeFileHandle(int handle) {
  if (handle < 0 || handle >= MAX_FILE_HANDLES) return;
  
  if (fileHandles[handle].inUse) {
    fileHandles[handle].file.close();
    fileHandles[handle].inUse = false;
  }
}

void KernelOS::freeDirHandle(int handle) {
  if (handle < 0 || handle >= MAX_DIR_HANDLES) return;
  
  if (dirHandles[handle].inUse) {
    dirHandles[handle].dir.close();
    dirHandles[handle].inUse = false;
  }
}

int KernelOS::fileOpen(const char* path, bool write) {
  if (!sdInitialized) return SYS_ERR_IO_ERROR;
  
  Task* current = getCurrentTask();
  if (!current->canAccessSD) return SYS_ERR_PERMISSION;
  
  int handle = allocateFileHandle();
  if (handle < 0) return SYS_ERR_NO_MEMORY;
  
  FileHandle* fh = &fileHandles[handle];
  fh->file = SD.open(path, write ? FILE_WRITE : FILE_READ);
  
  if (!fh->file) {
    return SYS_ERR_NOT_FOUND;
  }
  
  fh->inUse = true;
  fh->ownerTaskId = currentTaskId;
  fh->canWrite = write;
  current->fileHandles[handle] = true;
  
  return handle;
}

int KernelOS::fileClose(int handle) {
  if (handle < 0 || handle >= MAX_FILE_HANDLES) return SYS_ERR_INVALID_PARAM;
  if (!fileHandles[handle].inUse) return SYS_ERR_INVALID_PARAM;
  if (fileHandles[handle].ownerTaskId != currentTaskId) return SYS_ERR_PERMISSION;
  
  freeFileHandle(handle);
  getCurrentTask()->fileHandles[handle] = false;
  
  return SYS_OK;
}

int KernelOS::fileRead(int handle, void* buffer, size_t size) {
  if (handle < 0 || handle >= MAX_FILE_HANDLES) return SYS_ERR_INVALID_PARAM;
  if (!fileHandles[handle].inUse) return SYS_ERR_INVALID_PARAM;
  if (fileHandles[handle].ownerTaskId != currentTaskId) return SYS_ERR_PERMISSION;
  
  return fileHandles[handle].file.read((uint8_t*)buffer, size);
}

int KernelOS::fileWrite(int handle, const void* buffer, size_t size) {
  if (handle < 0 || handle >= MAX_FILE_HANDLES) return SYS_ERR_INVALID_PARAM;
  if (!fileHandles[handle].inUse) return SYS_ERR_INVALID_PARAM;
  if (fileHandles[handle].ownerTaskId != currentTaskId) return SYS_ERR_PERMISSION;
  if (!fileHandles[handle].canWrite) return SYS_ERR_PERMISSION;
  
  return fileHandles[handle].file.write((const uint8_t*)buffer, size);
}

bool KernelOS::fileDelete(const char* path) {
  if (!sdInitialized) return false;
  
  Task* current = getCurrentTask();
  if (!current->canAccessSD) return false;
  
  return SD.remove(path);
}

bool KernelOS::fileExists(const char* path) {
  if (!sdInitialized) return false;
  
  Task* current = getCurrentTask();
  if (!current->canAccessSD) return false;
  
  return SD.exists(path);
}

size_t KernelOS::fileSize(int handle) {
  if (handle < 0 || handle >= MAX_FILE_HANDLES) return 0;
  if (!fileHandles[handle].inUse) return 0;
  if (fileHandles[handle].ownerTaskId != currentTaskId) return 0;
  
  return fileHandles[handle].file.size();
}

// ============================================================================
// DIRECTORY OPERATIONS
// ============================================================================

int KernelOS::dirOpen(const char* path) {
  if (!sdInitialized) return SYS_ERR_IO_ERROR;
  
  Task* current = getCurrentTask();
  if (!current->canAccessSD) return SYS_ERR_PERMISSION;
  
  int handle = allocateDirHandle();
  if (handle < 0) return SYS_ERR_NO_MEMORY;
  
  DirHandle* dh = &dirHandles[handle];
  dh->dir = SD.open(path);
  
  if (!dh->dir) {
    return SYS_ERR_NOT_FOUND;
  }
  
  if (!dh->dir.isDirectory()) {
    dh->dir.close();
    return SYS_ERR_INVALID_PARAM;
  }
  
  dh->inUse = true;
  dh->ownerTaskId = currentTaskId;
  current->dirHandles[handle] = true;
  
  return handle;
}

int KernelOS::dirClose(int handle) {
  if (handle < 0 || handle >= MAX_DIR_HANDLES) return SYS_ERR_INVALID_PARAM;
  if (!dirHandles[handle].inUse) return SYS_ERR_INVALID_PARAM;
  if (dirHandles[handle].ownerTaskId != currentTaskId) return SYS_ERR_PERMISSION;
  
  freeDirHandle(handle);
  getCurrentTask()->dirHandles[handle] = false;
  
  return SYS_OK;
}

bool KernelOS::dirRead(int handle, DirEntry* entry) {
  if (handle < 0 || handle >= MAX_DIR_HANDLES) return false;
  if (!dirHandles[handle].inUse) return false;
  if (dirHandles[handle].ownerTaskId != currentTaskId) return false;
  if (!entry) return false;
  
  File nextEntry = dirHandles[handle].dir.openNextFile();
  if (!nextEntry) {
    return false;
  }
  
  strncpy(entry->name, nextEntry.name(), sizeof(entry->name) - 1);
  entry->name[sizeof(entry->name) - 1] = '\0';
  entry->isDirectory = nextEntry.isDirectory();
  entry->size = nextEntry.size();
  
  nextEntry.close();
  return true;
}

bool KernelOS::dirCreate(const char* path) {
  if (!sdInitialized) return false;
  
  Task* current = getCurrentTask();
  if (!current->canAccessSD) return false;
  
  return SD.mkdir(path);
}

bool KernelOS::dirRemove(const char* path) {
  if (!sdInitialized) return false;
  
  Task* current = getCurrentTask();
  if (!current->canAccessSD) return false;
  
  return SD.rmdir(path);
}

void KernelOS::dirRewind(int handle) {
  if (handle < 0 || handle >= MAX_DIR_HANDLES) return;
  if (!dirHandles[handle].inUse) return;
  if (dirHandles[handle].ownerTaskId != currentTaskId) return;
  
  dirHandles[handle].dir.rewindDirectory();
}

// ============================================================================
// SYSTEM CALLS
// ============================================================================

int KernelOS::syscall(SyscallType type, void* arg1, void* arg2, void* arg3, void* arg4) {
  switch (type) {
    // File operations
    case SYS_FILE_OPEN:
      return fileOpen((const char*)arg1, (bool)(intptr_t)arg2);
    case SYS_FILE_CLOSE:
      return fileClose((int)(intptr_t)arg1);
    case SYS_FILE_READ:
      return fileRead((int)(intptr_t)arg1, arg2, (size_t)(intptr_t)arg3);
    case SYS_FILE_WRITE:
      return fileWrite((int)(intptr_t)arg1, arg2, (size_t)(intptr_t)arg3);
    case SYS_FILE_DELETE:
      return fileDelete((const char*)arg1) ? SYS_OK : SYS_ERR_IO_ERROR;
    case SYS_FILE_EXISTS:
      return fileExists((const char*)arg1) ? 1 : 0;
    case SYS_FILE_SIZE:
      return (int)fileSize((int)(intptr_t)arg1);
    
    // Directory operations
    case SYS_DIR_OPEN:
      return dirOpen((const char*)arg1);
    case SYS_DIR_CLOSE:
      return dirClose((int)(intptr_t)arg1);
    case SYS_DIR_READ:
      return dirRead((int)(intptr_t)arg1, (DirEntry*)arg2) ? 1 : 0;
    case SYS_DIR_CREATE:
      return dirCreate((const char*)arg1) ? SYS_OK : SYS_ERR_IO_ERROR;
    case SYS_DIR_REMOVE:
      return dirRemove((const char*)arg1) ? SYS_OK : SYS_ERR_IO_ERROR;
    case SYS_DIR_REWIND:
      dirRewind((int)(intptr_t)arg1);
      return SYS_OK;
    
    // Memory operations
    case SYS_MEM_ALLOC:
      return (int)(intptr_t)memAlloc((size_t)(intptr_t)arg1);
    case SYS_MEM_FREE:
      memFree(arg1);
      return SYS_OK;
    case SYS_MEM_COMPACT:
      memCompact();
      return SYS_OK;
    
    // Task operations
    case SYS_TASK_YIELD:
      yield();
      return SYS_OK;
    case SYS_TASK_SLEEP:
      sleep((uint32_t)(intptr_t)arg1);
      return SYS_OK;
    
    // IPC operations
    case SYS_IPC_SEND:
      return ipcSend((int)(intptr_t)arg1, arg2, (size_t)(intptr_t)arg3);
    case SYS_IPC_RECEIVE:
      return ipcReceive(arg1, (size_t)(intptr_t)arg2, (int*)arg3);
    case SYS_IPC_POLL:
      return ipcPoll();
    
    // Semaphore operations
    case SYS_SEM_CREATE:
      return semCreate((int)(intptr_t)arg1, (int)(intptr_t)arg2, (const char*)arg3);
    case SYS_SEM_WAIT:
      return semWait((int)(intptr_t)arg1, (uint32_t)(intptr_t)arg2);
    case SYS_SEM_POST:
      return semPost((int)(intptr_t)arg1);
    case SYS_SEM_DESTROY:
      return semDestroy((int)(intptr_t)arg1);
    
    // GPIO operations
    case SYS_GPIO_PINMODE:
      return gpioSetMode((int)(intptr_t)arg1, (int)(intptr_t)arg2);
    case SYS_GPIO_WRITE:
      return gpioWrite((int)(intptr_t)arg1, (int)(intptr_t)arg2);
    case SYS_GPIO_READ:
      return gpioRead((int)(intptr_t)arg1);
    case SYS_GPIO_ANALOG_READ:
      return gpioAnalogRead((int)(intptr_t)arg1);
    case SYS_GPIO_ANALOG_WRITE:
      return gpioAnalogWrite((int)(intptr_t)arg1, (int)(intptr_t)arg2);
    
    // I2C operations
    case SYS_I2C_BEGIN:
      return i2cBegin((uint8_t)(intptr_t)arg1);
    case SYS_I2C_WRITE:
      return i2cWrite((uint8_t)(intptr_t)arg1, (const uint8_t*)arg2, (size_t)(intptr_t)arg3);
    case SYS_I2C_READ:
      return i2cRead((uint8_t)(intptr_t)arg1, (uint8_t*)arg2, (size_t)(intptr_t)arg3);
    case SYS_I2C_REQUEST:
      return i2cRequest((uint8_t)(intptr_t)arg1, (size_t)(intptr_t)arg2);
    
    // SPI operations
    case SYS_SPI_BEGIN:
      return spiBegin();
    case SYS_SPI_TRANSFER:
      return spiTransfer((uint8_t*)arg1, (uint8_t*)arg2, (size_t)(intptr_t)arg3);
    case SYS_SPI_END:
      return spiEnd();
    
    // System operations
    case SYS_GET_TIME:
      return (int)millis();
    
    default:
      return SYS_ERR_INVALID_CALL;
  }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void KernelOS::print(const char* message) {
  Serial.print(F("["));
  Serial.print(getCurrentTask()->name);
  Serial.print(F("] "));
  Serial.println(message);
}

void KernelOS::debug(const char* message) {
  Serial.print(F("[DEBUG] "));
  Serial.println(message);
}

uint32_t KernelOS::uptime() {
  return millis() - bootTime;
}

int KernelOS::getCurrentTaskId() {
  return currentTaskId;
}

void KernelOS::printTaskList(Text& textToPrint) {
  textToPrint.println(F("\n=== Task List ==="));
  textToPrint.println(F("ID  Name            State      Memory   LastYield"));
  textToPrint.println(F("--- --------------- ---------- -------- ---------"));
  
  for (int i = 0; i < MAX_TASKS; i++) {
    if (tasks[i].state == TASK_EMPTY) continue;
    
    textToPrint.print(i);
    textToPrint.print(F("   "));
    textToPrint.print(tasks[i].name);
    
    for (int j = strlen(tasks[i].name); j < 16; j++) {
      textToPrint.print(' ');
    }
    
    switch (tasks[i].state) {
      case TASK_READY: textToPrint.print(F("READY     ")); break;
      case TASK_RUNNING: textToPrint.print(F("RUNNING   ")); break;
      case TASK_SLEEPING: textToPrint.print(F("SLEEPING  ")); break;
      case TASK_BLOCKED: textToPrint.print(F("BLOCKED   ")); break;
      default: textToPrint.print(F("UNKNOWN   ")); break;
    }
    
    textToPrint.print(tasks[i].memoryUsed);
    textToPrint.print(F(" B    "));
    
    uint32_t timeSinceYield = millis() - tasks[i].lastYield;
    textToPrint.print(timeSinceYield);
    textToPrint.println(F("ms"));
  }
  textToPrint.println();
}

void KernelOS::printTaskList() {
  Serial.println(F("\n=== Task List ==="));
  Serial.println(F("ID  Name            State      Memory   LastYield"));
  Serial.println(F("--- --------------- ---------- -------- ---------"));
  
  for (int i = 0; i < MAX_TASKS; i++) {
    if (tasks[i].state == TASK_EMPTY) continue;
    
    Serial.print(i);
    Serial.print(F("   "));
    Serial.print(tasks[i].name);
    
    for (int j = strlen(tasks[i].name); j < 16; j++) {
      Serial.print(' ');
    }
    
    switch (tasks[i].state) {
      case TASK_READY: Serial.print(F("READY     ")); break;
      case TASK_RUNNING: Serial.print(F("RUNNING   ")); break;
      case TASK_SLEEPING: Serial.print(F("SLEEPING  ")); break;
      case TASK_BLOCKED: Serial.print(F("BLOCKED   ")); break;
      default: Serial.print(F("UNKNOWN   ")); break;
    }
    
    Serial.print(tasks[i].memoryUsed);
    Serial.print(F(" B    "));
    
    uint32_t timeSinceYield = millis() - tasks[i].lastYield;
    Serial.print(timeSinceYield);
    Serial.println(F("ms"));
  }
  Serial.println();
}

void KernelOS::printMemoryInfo(Text& textToPrint) {
  textToPrint.println(F("\n=== Memory Info ==="));
  textToPrint.print(F("Total heap:     "));
  textToPrint.print(KERNEL_HEAP_SIZE);
  textToPrint.println(F(" bytes"));
  textToPrint.print(F("Used:           "));
  textToPrint.print(heapUsed);
  textToPrint.println(F(" bytes"));
  textToPrint.print(F("Available:      "));
  textToPrint.print(memAvailable());
  textToPrint.println(F(" bytes"));
  
  // Count blocks
  size_t readPos = 0;
  int usedBlocks = 0;
  int freeBlocks = 0;
  
  while (readPos < heapUsed) {
    MemoryBlock* block = (MemoryBlock*)&kernelHeap[readPos];
    if (block->inUse) {
      usedBlocks++;
    } else {
      freeBlocks++;
    }
    readPos += sizeof(MemoryBlock) + block->size;
  }
  
  textToPrint.print(F("Used blocks:    "));
  textToPrint.println(usedBlocks);
  textToPrint.print(F("Free blocks:    "));
  textToPrint.println(freeBlocks);
  if (freeBlocks > 0) {
    textToPrint.println(F("Fragmentation detected - consider compacting"));
  }
  textToPrint.println();
}


void KernelOS::printMemoryInfo() {
  Serial.println(F("\n=== Memory Info ==="));
  Serial.print(F("Total heap:     "));
  Serial.print(KERNEL_HEAP_SIZE);
  Serial.println(F(" bytes"));
  Serial.print(F("Used:           "));
  Serial.print(heapUsed);
  Serial.println(F(" bytes"));
  Serial.print(F("Available:      "));
  Serial.print(memAvailable());
  Serial.println(F(" bytes"));
  
  // Count blocks
  size_t readPos = 0;
  int usedBlocks = 0;
  int freeBlocks = 0;
  
  while (readPos < heapUsed) {
    MemoryBlock* block = (MemoryBlock*)&kernelHeap[readPos];
    if (block->inUse) {
      usedBlocks++;
    } else {
      freeBlocks++;
    }
    readPos += sizeof(MemoryBlock) + block->size;
  }
  
  Serial.print(F("Used blocks:    "));
  Serial.println(usedBlocks);
  Serial.print(F("Free blocks:    "));
  Serial.println(freeBlocks);
  if (freeBlocks > 0) {
    Serial.println(F("Fragmentation detected - consider compacting"));
  }
  Serial.println();
}

void KernelOS::temporaryDebugYield()
{
    Task* current = getCurrentTask();
    if (current) {
        // current->state = TASK_READY;
        current->lastYield = millis();
    }
}













///////////////////////////////////////////////
//      part that was in the shell.ino       //
///////////////////////////////////////////////

















/*
  Shell Application - Full Featured with Directory Support
*/


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

  static uint8_t taskState = 1;//this dictates if we are currently initializing (1), running (2), or terminating (3)
  static ShellState* state = (ShellState*)OS::malloc(sizeof(ShellState));// Allocate shell state through kernel heap

  while(true)
  {

    //are we initializing this task for the first time
    if(taskState == 1/*Initializing*/)
    {
        
        
        // Initialize state
        strcpy(state->currentDir, "/");
        state->cmdLen = 0;
        state->terminal = &ui.addTerminal("terminal", "middle", "middle", 200, 200, color(0, 0, 0), color(0, 255, 0));//create a terminal in the main ui

        state->terminal->deathCallbackInput = &taskState;
        state->terminal->deathCallback = killShell;

        // Print initial prompt
        printPrompt(state->currentDir, state->terminal->text);

        //set the state to running
        taskState = 2/*Running*/;
    }

    //if we are currently running the terminal/shell task
    if(taskState == 2/*Running*/)
    {
        //check if we got a new command
        if(state->terminal->cmdAvailable)
        {
            
            state->terminal->cmdAvailable = 0;//put back the available 
            processCommand(state->terminal->cmdBuffer, state->currentDir, state->terminal->text);
            printPrompt(state->currentDir, state->terminal->text);
        }
    }
    
    
    // Never reached, but good practice
    if(taskState == 3/*Terminated*/)
    {
        //free the variables from memory (we aren,t using them anymore)
        OS::free(state);
        break;
    }

    //update the uis (temporary, just wanna see if it fixes my bug)
    //draw on the screen buffer
    ui.update();

    //don't render out to the  if we are waiting of keyboard input
    if(!kbd.Available)
    {
        displayFrameBuffer();//render on the screen
    }


  }

  //kill the task
  KernelOS::killTask(KernelOS::getCurrentTaskId());

  
//   OS::temporaryDebugYield();//temporary debug yield function to keep watchdog from going crazy
}

//function to update the ui and the screen
void updateDisplayTask()
{
  println("UPDATING THE DISPLAY");
  //draw on the screen buffer
  ui.update();

  //don't render out to the  if we are waiting of keyboard input
  if(!kbd.Available)
  {
    displayFrameBuffer();//render on the screen
  }

  OS::temporaryDebugYield();//temporary debug yield function to keep watchdog from going crazy
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
  for (int i = 0; i < 50; i++) {
    textToPrint.println();
  }
  textToPrint.println(F("================================="));
  textToPrint.println(F("  YandereOS Shell"));
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
