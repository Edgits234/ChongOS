
#include <kernelOS.h>

// ============================================================================
// MAIN PROGRAM
// ============================================================================

void setup() {
  Wire.begin();
  
  // Initialize kernel
  if (!KernelOS::init()) {
    Serial.println(F("FATAL: Kernel init failed"));
    while(1);
  }
  
  // Create shell task
  int shellTaskId = KernelOS::createTask("shell", shellTask);
  
  // Create screen update task
  int updateDisplayTaskId = KernelOS::createTask("display", updateDisplayTask);
  
  if (shellTaskId < 0) {
    KernelOS::panic("Failed to create shell task");
  }
  
  Serial.println(F("Shell ready. Type 'help' for commands.\n"));
}

void loop() {
  KernelOS::schedule();
  delay(1);
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
