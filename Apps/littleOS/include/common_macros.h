// GPIO02 Pad Macros (just to avoid collisions)

#define GPIO2_00            0   // --- IGNORE ---
#define GPIO2_01            1   // --- IGNORE ---
#define ULTRASONIC_TRIG_PIN 2   // GPIO2_02
#define ULTRASONIC_ECHO_PIN 3   // GPIO2_03
#define BUILTIN_GREEN_LED   4   // GPIO2_04
#define GPIO2_5             5   // --- IGNORE ---
#define GPIO2_6             6   // --- IGNORE ---
#define GPIO2_7             7   // --- IGNORE ---
#define GPIO2_8             8   // --- IGNORE ---
#define GPIO2_9             9   // --- IGNORE ---
#define GPIO2_10            10  // --- IGNORE ---
#define GPIO2_11            11  // --- IGNORE ---
#define BUILTIN_BLUE_LED    12  // GPIO2_12
#define BUILTIN_RED_LED     13  // GPIO2_13
#define ESP8266_TX_PIN      14  // GPIO2_14
#define ESP8266_RX_PIN      15  // GPIO2_15
#define GPIO2_16            16  // --- IGNORE ---
#define GPIO2_17            17  // --- IGNORE ---
#define GPIO2_18            18  // --- IGNORE ---
#define GPIO2_19            19  // --- IGNORE ---
#define GPIO2_20            20  // --- IGNORE ---
#define GPIO2_21            21  // --- IGNORE ---
#define GPIO2_22            22  // --- IGNORE ---
#define GPIO2_23            23  // --- IGNORE ---
#define GPIO2_24            24  // --- IGNORE ---
#define GPIO2_25            25  // --- IGNORE ---
#define GPIO2_26            26  // --- IGNORE ---
#define GPIO2_27            27  // --- IGNORE ---
#define GPIO2_28            28  // --- IGNORE ---
#define GPIO2_29            29  // --- IGNORE ---
#define GPIO2_30            30  // --- IGNORE ---
#define GPIO2_31            31  // --- IGNORE ---



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