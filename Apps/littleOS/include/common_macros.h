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

// gic's Macros
#define GICD_BASE       0x48000000ULL // Base address for GIC Distributor
#define GICR_BASE       0x48040000ULL // Base address for GIC Redistributor
#define GICR_SGI_BASE   0x48050000ULL // Base address for GIC Redistributor SGI registers
#define GICD_CTLR       (*(volatile uint32_t*)(GICD_BASE + 0x0000)) // GIC Distributor Control Register
#define GICR_WAKER      (*(volatile uint32_t*)(GICR_BASE + 0x0014)) // GIC Redistributor WAKER Register

// irq's Macros

// multitasking's Macros
#define THREAD_STACK_SIZE   16*(1<<10) // 16 KB of stack per thread
#define MAX_THREADS         16 // total max threads allowed in system
#define RR_TIME_QUANTUM_MS  20 // Time quantum for Round Robin scheduling in milliseconds

// shared_locks's Macros

// stdio's Macros

// string's Macros

// timer's Macros