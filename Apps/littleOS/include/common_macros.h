// cli's Macros
#define CMD_BUFFER_SIZE 128 // CLI command parsing buffer size
#define ASCII_CTRL_C        0x03 // ASCII code for Ctrl+C
#define ASCII_BACKSPACE     0x08 // ASCII code for Backspace
#define ASCII_DEL           0x7F // ASCII code for Delete
#define CLI_HISTORY_SIZE    10   // Number of commands to store in cmd_history

// datetime's Macros

// esp8266's Macros
#define IMX91_LPUART4_IRQ_ID    101  // GIC IRQ ID = SPI Number + 32 and in manual SPI Number is 69
#define ESP_RX_BUFFER_SIZE      2048 // Size of the ring buffer for incoming data from ESP8266

// irq's Macros

// multitasking's Macros
#define THREAD_STACK_SIZE   16*(1<<10) // 16 KB of stack per thread
#define MAX_THREADS         16 // total max threads allowed in system
#define RR_TIME_QUANTUM_MS  20 // Time quantum for Round Robin scheduling in milliseconds

// shared_locks's Macros

// stdio's Macros

// string's Macros

// timer's Macros