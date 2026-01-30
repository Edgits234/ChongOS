/*
  ChongOS Kernel V4.0
  Implementation File
*/
//KERNELOS.cpp FILE

#include "kernelOS.h"
#include "mbed.h"

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

// Mbed OS IPC primitives
rtos::Mail<Message, 16> KernelOS::mailQueues[MAX_TASKS];
rtos::Semaphore* KernelOS::semaphores[MAX_SEMAPHORES];
bool KernelOS::semaphoreInUse[MAX_SEMAPHORES];
uint8_t KernelOS::mailQueueCounts[MAX_TASKS];  // Track message counts

// Thread mapping for preemptive multitasking
rtos::Mutex KernelOS::threadMapMutex;
osThreadId_t KernelOS::threadToTaskMap[MAX_TASKS];

bool KernelOS::watchdogEnabled = true;
uint32_t KernelOS::watchdogLastCheck = 0;

bool KernelOS::initialized = false;
uint32_t KernelOS::bootTime = 0;

// Initialize mutexes
rtos::Mutex KernelOS::heapMutex;
rtos::Mutex KernelOS::sdMutex;
rtos::Mutex KernelOS::displayMutex;
rtos::Mutex KernelOS::serialMutex;

// Health monitoring initialization
uint32_t KernelOS::taskLastActivity[MAX_TASKS];
uint8_t KernelOS::taskCrashCount[MAX_TASKS];

// Display output pointer
Text* KernelOS::kernelDisplayOutput = nullptr;

// ============================================================================
// INITIALIZATION
// ============================================================================

bool KernelOS::init() {
  if (initialized) return true;
  
  Serial.begin(9600);
  while (!Serial && millis() < 14000) delay(1000);
  delay(500);
  
  if (kernelDisplayOutput) {
    kernelDisplayOutput->println(F("\n=== ChongOS Kernel v4.0 ==="));
    kernelDisplayOutput->println(F("Initializing..."));
  }

  // Initialize the ui library
  ui.basecolor = color(50, 50, 50);  //change the base color (before begin shows up the background)
  ui.begin();
  
  // Initialize all tasks as empty
  for (int i = 0; i < MAX_TASKS; i++) {
    tasks[i].id = -1;
    tasks[i].thread = nullptr;
    taskLastActivity[i] = millis();
    taskCrashCount[i] = 0;
  }
  
  // Initialize file handles
  for (int i = 0; i < MAX_FILE_HANDLES; i++) {
    fileHandles[i].inUse = false;
  }
  
  // Initialize directory handles
  for (int i = 0; i < MAX_DIR_HANDLES; i++) {
    dirHandles[i].inUse = false;
  }
  
  // Initialize semaphores as null and unused
  for (int i = 0; i < MAX_SEMAPHORES; i++) {
    semaphores[i] = nullptr;
    semaphoreInUse[i] = false;
  }
  // Initialize message queue counts
  for (int i = 0; i < MAX_TASKS; i++) {
    mailQueueCounts[i] = 0;
  }
  
  // Initialize memory
  heapUsed = 0;
  
  // Initialize SD card
  if (kernelDisplayOutput) kernelDisplayOutput->println(F("Mounting SD card... "));
  if (SD.begin(SD_CS_PIN)) {
    sdInitialized = true;
    if (kernelDisplayOutput) kernelDisplayOutput->println(F("OK"));
  } else {
    if (kernelDisplayOutput) {
      kernelDisplayOutput->println(F("FAILED"));
      kernelDisplayOutput->println(F("Warning: SD card not available"));
    }
  }
  
  // Create idle task (task 0)
  tasks[0].id = 0;
  tasks[0].name = "idle";
  tasks[0].priority = 0;
  tasks[0].thread = nullptr;  // Idle task thread will be created by Mbed
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
  
  if (kernelDisplayOutput) kernelDisplayOutput->println(F("Kernel initialized successfully\n"));
  return true;
}

void KernelOS::panic(const char* message) {
  if (kernelDisplayOutput) {
    kernelDisplayOutput->println(F("\n!!! KERNEL PANIC !!!"));
    kernelDisplayOutput->println(message);
  }
  
  // Print current task info
  Task* current = getCurrentTask();
  if (current && kernelDisplayOutput) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Current task: %s (ID: %d)", current->name, current->id);
    kernelDisplayOutput->println(buffer);
    
    // Print stack trace if available
    printStackTrace(current);
  }
  
  if (kernelDisplayOutput) kernelDisplayOutput->println(F("\n=== System State ==="));
  printTaskList();
  printMemoryInfo();
  
  if (kernelDisplayOutput) kernelDisplayOutput->println(F("\nSystem halted."));
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
    if (kernelDisplayOutput) kernelDisplayOutput->println(F("No stack trace available"));
    return;
  }
  
  if (kernelDisplayOutput) kernelDisplayOutput->println(F("\n=== Stack Trace ==="));
  for (int i = 0; i < task->stackTraceDepth; i++) {
    char buffer[128];
    if (task->stackTrace[i].functionName) {
      snprintf(buffer, sizeof(buffer), "  [%d] %s @ 0x%lx", i, task->stackTrace[i].functionName, (uintptr_t)task->stackTrace[i].returnAddress);
    } else {
      snprintf(buffer, sizeof(buffer), "  [%d] <unknown> @ 0x%lx", i, (uintptr_t)task->stackTrace[i].returnAddress);
    }
    if (kernelDisplayOutput) kernelDisplayOutput->println(buffer);
  }
}

// ============================================================================
// TASK MANAGEMENT
// ============================================================================

int KernelOS::allocateTaskId() {
  for (int i = 1; i < MAX_TASKS; i++) {
    if (tasks[i].id == -1) {
      return i;
    }
  }
  return -1;
}

int KernelOS::getCurrentTaskId() {
  osThreadId_t currentThread = osThreadGetId();
  
  threadMapMutex.lock();
  for (int i = 0; i < MAX_TASKS; i++) {
    if (threadToTaskMap[i] == currentThread) {
      threadMapMutex.unlock();
      return i;
    }
  }
  threadMapMutex.unlock();
  
  // Should not reach here - return task 0 (kernel task)
  return 0;
}

Task* KernelOS::getCurrentTask() {
  int taskId = getCurrentTaskId();
  return &tasks[taskId];
}

Task* KernelOS::getTask(int taskId) {
  if (taskId < 0 || taskId >= MAX_TASKS) return nullptr;
  if (tasks[taskId].id == -1) return nullptr;
  return &tasks[taskId];
}

int KernelOS::createTask(const char* name, void (*entryPoint)()) {
    int taskId = allocateTaskId();
    if (taskId < 0) return SYS_ERR_NO_MEMORY;

    Task* task = &tasks[taskId];
    task->id = taskId;
    task->name = name;
    task->entryPoint = entryPoint;
    task->priority = 10;

    // Create actual Mbed thread (32KB stack - plenty of room for nested calls)
    task->thread = new rtos::Thread(osPriorityNormal, 32768);  // 32KB stack
    task->thread->start(entryPoint);
    
    // Map thread ID to task ID for later retrieval
    threadMapMutex.lock();
    threadToTaskMap[taskId] = task->thread->get_id();
    threadMapMutex.unlock();

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
    
    char buffer[96];
    snprintf(buffer, sizeof(buffer), "Task created: %s (ID: %d)", name, taskId);
    if (kernelDisplayOutput) kernelDisplayOutput->println(buffer);

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
  
  // Terminate the Mbed thread
  if (task->thread) {
    task->thread->terminate();
    delete task->thread;
    task->thread = nullptr;
  }
  
  // Clean up thread mapping
  threadMapMutex.lock();
  threadToTaskMap[taskId] = 0;
  threadMapMutex.unlock();
  
  task->id = -1;
  
  char buffer[96];
  snprintf(buffer, sizeof(buffer), "Task killed: %s", task->name);
  if (kernelDisplayOutput) kernelDisplayOutput->println(buffer);
}

// ============================================================================
// HEALTH MONITORING AND AUTO-RECOVERY
// ============================================================================

void KernelOS::recordTaskActivity(int taskId) {
  if (taskId < 0 || taskId >= MAX_TASKS) return;
  taskLastActivity[taskId] = millis();
}

bool KernelOS::attemptTaskRecovery(int taskId) {
  if (taskId < 0 || taskId >= MAX_TASKS) return false;
  if (tasks[taskId].id == -1) return false;  // Task doesn't exist
  
  Task* task = &tasks[taskId];
  taskCrashCount[taskId]++;
  
  if (taskCrashCount[taskId] > MAX_RECOVERY_ATTEMPTS) {
    // Too many crashes, trigger full system reboot
    char msg[96];
    snprintf(msg, sizeof(msg), "FATAL: Task %s crashed %d times, rebooting system", 
            task->name, taskCrashCount[taskId]);
    panic(msg);
    return false;
  }
  
  // Attempt recovery
  char buffer[96];
  snprintf(buffer, sizeof(buffer), "[Recovery #%d] Restarting task: %s", taskCrashCount[taskId], task->name);
  if (kernelDisplayOutput) kernelDisplayOutput->println(buffer);
  
  // Kill the task
  killTask(taskId);
  
  // Restart it
  int newTaskId = createTask(task->name, task->entryPoint);
  
  // Reset activity timer
  if (newTaskId >= 0) {
    taskLastActivity[newTaskId] = millis();
    return true;
  }
  
  return false;
}

void KernelOS::monitorTaskHealth() {
  static uint32_t lastCheck = millis();
  
  uint32_t now = millis();
  
  // Check every 1 second
  if ((now - lastCheck) < 1000) {
    return;
  }
  lastCheck = now;
  
  // Check each task
  for (int i = 1; i < MAX_TASKS; i++) {
    if (tasks[i].id == -1) continue;  // Empty slot, skip
    
    uint32_t elapsed = now - taskLastActivity[i];
    
    // If task hasn't reported in for TASK_TIMEOUT_MS
    if (elapsed > TASK_TIMEOUT_MS) {
      char buffer[96];
      snprintf(buffer, sizeof(buffer), "[WARNING] Task %s appears frozen for %lums", tasks[i].name, elapsed);
      if (kernelDisplayOutput) kernelDisplayOutput->println(buffer);
      
      // Attempt recovery
      attemptTaskRecovery(i);
    }
  }
  
  // Keep watchdog alive - this function is called from main thread
  Watchdog::get_instance().kick();
}

// ============================================================================
// SCHEDULER
// ============================================================================

void KernelOS::schedule() {
    // Mbed's scheduler runs automatically
}

// ============================================================================
// YIELD AND SLEEP
// ============================================================================

void KernelOS::yield() {
    rtos::ThisThread::yield();
}

void KernelOS::sleep(uint32_t ms) {
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(ms));
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
    if (kernelDisplayOutput) kernelDisplayOutput->println(F("[Memory] Out of space, compacting..."));
    compactMemory();
    
    if (heapUsed + totalNeeded > KERNEL_HEAP_SIZE) {
      if (kernelDisplayOutput) kernelDisplayOutput->println(F("[Memory] Out of memory after compaction!"));
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
    if (kernelDisplayOutput) kernelDisplayOutput->println(F("[Memory] Warning: Invalid free()"));
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
  
  if (kernelDisplayOutput) kernelDisplayOutput->println(F("[Memory] Compacting heap (WARNING: may invalidate pointers)"));
  
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
  
  if (kernelDisplayOutput) {
    char buffer[96];
    snprintf(buffer, sizeof(buffer), "[Memory] Compaction complete: freed %u bytes, moved %d blocks", (unsigned int)freedSpace, movedBlocks);
    kernelDisplayOutput->println(buffer);
  }
  
  if (movedBlocks > 0 && kernelDisplayOutput) {
    kernelDisplayOutput->println(F("[Memory] WARNING: Existing pointers may be invalid!"));
    kernelDisplayOutput->println(F("[Memory] Recommendation: Free and reallocate after compaction"));
  }
}

void* KernelOS::memAlloc(size_t size) {
    heapMutex.lock();
    void* ptr = allocateMemoryInternal(size, getCurrentTaskId());
    heapMutex.unlock();
    
    if (!ptr && size > 0) {
        char errMsg[80];
        snprintf(errMsg, sizeof(errMsg), "OUT_OF_MEMORY: Failed to allocate %d bytes", (int)size);
        panic(errMsg);
    }
    
    return ptr;
}

void KernelOS::memFree(void* ptr) {
    heapMutex.lock();
    freeMemoryInternal(ptr);
    heapMutex.unlock();
}

void KernelOS::memCompact() {
    heapMutex.lock();
    compactMemory();
    heapMutex.unlock();
}

// ============================================================================
// IPC - MESSAGE QUEUES
// ============================================================================

int KernelOS::ipcSend(int toTaskId, const void* data, size_t length) {
    if (toTaskId < 0 || toTaskId >= MAX_TASKS) return SYS_ERR_INVALID_PARAM;

    Message* msg = mailQueues[toTaskId].try_alloc();
    if (!msg) return SYS_ERR_NO_MEMORY;

    msg->fromTaskId = getCurrentTaskId();
    msg->length = length;
    memcpy(msg->data, data, length);
    msg->timestamp = millis();

    mailQueues[toTaskId].put(msg);
    mailQueueCounts[toTaskId]++;  // Increment message count
    return SYS_OK;
}

int KernelOS::ipcReceive(void* buffer, size_t maxLength, int* fromTaskId) {
    int currentTaskId = getCurrentTaskId();
    osEvent evt = mailQueues[currentTaskId].get(0);  // Non-blocking
    if (evt.status != osEventMail) return SYS_ERR_WOULD_BLOCK;

    Message* msg = (Message*)evt.value.p;

    if (msg->length > maxLength) {
        mailQueues[currentTaskId].free(msg);
        return SYS_ERR_INVALID_PARAM;
    }

    memcpy(buffer, msg->data, msg->length);
    if (fromTaskId) *fromTaskId = msg->fromTaskId;
    int length = msg->length;

    mailQueues[currentTaskId].free(msg);
    mailQueueCounts[currentTaskId]--;  // Decrement message count
    return length;
}

int KernelOS::ipcPoll() {
    // Returns number of messages waiting in current task's queue (without consuming them)
    int currentTaskId = getCurrentTaskId();
    return mailQueueCounts[currentTaskId];
}

// ============================================================================
// IPC - SEMAPHORES
// ============================================================================

int KernelOS::allocateSemaphore() {
  for (int i = 0; i < MAX_SEMAPHORES; i++) {
    if (!semaphoreInUse[i]) {
      return i;
    }
  }
  return -1;
}

int KernelOS::semCreate(int initialValue, int maxValue, const char* name) {
    int semId = allocateSemaphore();
    if (semId < 0) return SYS_ERR_NO_MEMORY;

    semaphores[semId] = new rtos::Semaphore(maxValue, initialValue);
    semaphoreInUse[semId] = true;

    return semId;
}

int KernelOS::semWait(int semId, uint32_t timeoutMs) {
    if (semId < 0 || semId >= MAX_SEMAPHORES) return SYS_ERR_INVALID_PARAM;
    if (!semaphoreInUse[semId]) return SYS_ERR_NOT_FOUND;

    if (timeoutMs == 0) {
        // Try to acquire without blocking
        if (semaphores[semId]->try_acquire()) {
            return SYS_OK;
        } else {
            return SYS_ERR_WOULD_BLOCK;
        }
    } else {
        // Wait with timeout
        if (semaphores[semId]->try_acquire_for(std::chrono::milliseconds(timeoutMs))) {
            return SYS_OK;
        } else {
            return SYS_ERR_TIMEOUT;
        }
    }
}

int KernelOS::semPost(int semId) {
    if (semId < 0 || semId >= MAX_SEMAPHORES) return SYS_ERR_INVALID_PARAM;
    if (!semaphoreInUse[semId]) return SYS_ERR_NOT_FOUND;

    semaphores[semId]->release();
    return SYS_OK;
}

int KernelOS::semDestroy(int semId) {
  if (semId < 0 || semId >= MAX_SEMAPHORES) return SYS_ERR_INVALID_PARAM;
  if (!semaphoreInUse[semId]) return SYS_ERR_NOT_FOUND;
  
  // Delete the Mbed semaphore and mark as unused
  delete semaphores[semId];
  semaphores[semId] = nullptr;
  semaphoreInUse[semId] = false;
  
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

    sdMutex.lock();

    int handle = allocateFileHandle();
    if (handle < 0) {
        sdMutex.unlock();
        return SYS_ERR_NO_MEMORY;
    }

    FileHandle* fh = &fileHandles[handle];
    fh->file = SD.open(path, write ? FILE_WRITE : FILE_READ);

    if (!fh->file) {
        sdMutex.unlock();
        return SYS_ERR_IO_ERROR;
    }

    fh->inUse = true;
    fh->ownerTaskId = currentTaskId;
    fh->canWrite = write;
    current->fileHandles[handle] = true;

    sdMutex.unlock();
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
  if (fileHandles[handle].ownerTaskId != getCurrentTaskId()) return SYS_ERR_PERMISSION;
  
  sdMutex.lock();
  int result = fileHandles[handle].file.read((uint8_t*)buffer, size);
  sdMutex.unlock();
  
  return result;
}

int KernelOS::fileWrite(int handle, const void* buffer, size_t size) {
  if (handle < 0 || handle >= MAX_FILE_HANDLES) return SYS_ERR_INVALID_PARAM;
  if (!fileHandles[handle].inUse) return SYS_ERR_INVALID_PARAM;
  if (fileHandles[handle].ownerTaskId != getCurrentTaskId()) return SYS_ERR_PERMISSION;
  if (!fileHandles[handle].canWrite) return SYS_ERR_PERMISSION;
  
  sdMutex.lock();
  int result = fileHandles[handle].file.write((const uint8_t*)buffer, size);
  sdMutex.unlock();
  
  return result;
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
  if (fileHandles[handle].ownerTaskId != getCurrentTaskId()) return 0;
  
  sdMutex.lock();
  size_t result = fileHandles[handle].file.size();
  sdMutex.unlock();
  
  return result;
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
  if (dirHandles[handle].ownerTaskId != getCurrentTaskId()) return false;
  if (!entry) return false;
  
  sdMutex.lock();
  
  File nextEntry = dirHandles[handle].dir.openNextFile();
  if (!nextEntry) {
    sdMutex.unlock();
    return false;
  }
  
  strncpy(entry->name, nextEntry.name(), sizeof(entry->name) - 1);
  entry->name[sizeof(entry->name) - 1] = '\0';
  entry->isDirectory = nextEntry.isDirectory();
  entry->size = nextEntry.size();
  
  nextEntry.close();
  sdMutex.unlock();
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

void KernelOS::setDisplayOutput(Text* display) {
  kernelDisplayOutput = display;
}

void KernelOS::print(const char* message) {
  char buffer[128];
  snprintf(buffer, sizeof(buffer), "[%s] %s", getCurrentTask()->name, message);
  if (kernelDisplayOutput) {
    kernelDisplayOutput->println(buffer);
  }
}

void KernelOS::debug(const char* message) {
  char buffer[128];
  snprintf(buffer, sizeof(buffer), "[DEBUG] %s", message);
  if (kernelDisplayOutput) {
    kernelDisplayOutput->println(buffer);
  }
}

uint32_t KernelOS::uptime() {
  return millis() - bootTime;
}

int KernelOS::getCurrentTaskId() {
  return currentTaskId;
}

void KernelOS::printTaskList(Text& textToPrint) {
  textToPrint.println(F("\n=== Task List ==="));
  textToPrint.println(F("ID  Name            State      Memory"));
  textToPrint.println(F("--- --------------- ---------- --------"));
  
  for (int i = 0; i < MAX_TASKS; i++) {
    if (tasks[i].id == -1) continue;
    
    textToPrint.print(i);
    textToPrint.print(F("   "));
    textToPrint.print(tasks[i].name);
    
    for (int j = strlen(tasks[i].name); j < 16; j++) {
      textToPrint.print(' ');
    }
    
    // With Mbed OS, threads manage their own state
    if (tasks[i].thread != nullptr) {
      textToPrint.print(F("RUNNING    "));
    } else {
      textToPrint.print(F("READY      "));
    }
    
    textToPrint.print(tasks[i].memoryUsed);
    textToPrint.println(F(" B"));
  }
  textToPrint.println();
}

void KernelOS::printTaskList() {
  if (!kernelDisplayOutput) return;
  
  kernelDisplayOutput->println(F("\n=== Task List ==="));
  kernelDisplayOutput->println(F("ID  Name            State      Memory"));
  kernelDisplayOutput->println(F("--- --------------- ---------- --------"));
  
  for (int i = 0; i < MAX_TASKS; i++) {
    if (tasks[i].id == -1) continue;
    
    char buffer[96];
    snprintf(buffer, sizeof(buffer), "%d   %-16s ", i, tasks[i].name);

    // With Mbed OS, threads manage their own state
    if (tasks[i].thread != nullptr) {
      strcat(buffer, "RUNNING    ");
    } else {
      strcat(buffer, "READY      ");
    }

    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "%u B", (unsigned int)tasks[i].memoryUsed);
    kernelDisplayOutput->println(buffer);
  }
  kernelDisplayOutput->println(F(""));
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
  if (!kernelDisplayOutput) return;
  
  kernelDisplayOutput->println(F("\n=== Memory Info ==="));
  
  char buffer[96];
  snprintf(buffer, sizeof(buffer), "Total heap:     %u bytes", KERNEL_HEAP_SIZE);
  kernelDisplayOutput->println(buffer);

  snprintf(buffer, sizeof(buffer), "Used:           %u bytes", (unsigned int)heapUsed);
  kernelDisplayOutput->println(buffer);

  snprintf(buffer, sizeof(buffer), "Available:      %u bytes", (unsigned int)memAvailable());
  kernelDisplayOutput->println(buffer);
  
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
  
  snprintf(buffer, sizeof(buffer), "Used blocks:    %d", usedBlocks);
  kernelDisplayOutput->println(buffer);

  snprintf(buffer, sizeof(buffer), "Free blocks:    %d", freeBlocks);
  kernelDisplayOutput->println(buffer);
  
  if (freeBlocks > 0) {
    kernelDisplayOutput->println(F("Fragmentation detected - consider compacting"));
  }
  kernelDisplayOutput->println(F(""));
}

void KernelOS::temporaryDebugYield()
{
    // This function is deprecated with Mbed OS integration
    // Mbed's scheduler handles yielding automatically
}













///////////////////////////////////////////////
//      part that was in the shell.ino       //
///////////////////////////////////////////////


