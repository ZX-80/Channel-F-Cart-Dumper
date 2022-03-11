constexpr uint8_t PHI = 40; // Time for a single clock cycle in microseconds. 0.5 in Channel F
constexpr uint8_t WRITE_PIN = 34;
constexpr uint8_t PHI_PIN = 38;
constexpr uint8_t DBUS_PINS[8] = {51, 49, 39, 35, 36, 32, 30, 28}; //D0 - D7
constexpr uint8_t ROMC_PINS[5] = {45, 43, 41, 37, 40}; //ROMC0 - ROMC4

void setup() {
    Serial.begin(9600);

    // Initialize cartridge pins
    digitalWrite(PHI_PIN, LOW);
    pinMode(PHI_PIN, OUTPUT);

    digitalWrite(WRITE_PIN, LOW);
    pinMode(WRITE_PIN, OUTPUT);

    for (uint8_t i = 0; i < 8; i++) {
        pinMode(DBUS_PINS[i], INPUT_PULLUP); // Default to high impedance
    }
    for (uint8_t i = 0; i < 5; i++) {
        digitalWrite(ROMC_PINS[i], LOW); // Default to ROMC 0
        pinMode(ROMC_PINS[i], OUTPUT);
    }

    // Dump cartridge ROM in read-only mode
    dump_cartridge(true); 
}

void loop() {}



// Sets the current ROMC command
void set_romc(uint8_t new_romc) {
    for (uint8_t i = 0; i < 5; i++) {
        digitalWrite(ROMC_PINS[i], bitRead(new_romc, i));
    }
}

// Retrieves the current value on the data bus
uint8_t read_dbus(void) {
    uint8_t dbus_data;
    for (uint8_t i = 0; i < 8; i++) {
        bitWrite(dbus_data, i, digitalRead(DBUS_PINS[i]));
    }
    return dbus_data;
}

// Execute a ROMC command that puts data on the data bus
void execute_out(uint8_t romc, uint8_t data) { // Short cycle (4 ticks)
    // After 1 tick, output the ROMC command
    clock_tick(1);
    set_romc(romc);

    // 3 ticks before the cycle ends, write to the data bus
    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(DBUS_PINS[i], bitRead(data, i));
        pinMode(DBUS_PINS[i], OUTPUT);
    }
    clock_tick(2);
    
    // Toggle WRITE on the last tick
    digitalWrite(WRITE_PIN, HIGH);
    clock_tick(1);
    digitalWrite(WRITE_PIN, LOW);

    // Free the data bus
    for (uint8_t i = 0; i < 8; i++) {
        pinMode(DBUS_PINS[i], INPUT_PULLUP);
    }
}

// Execute a ROMC command that recieves data from the data bus
uint8_t execute_in(uint8_t romc) { // Short cycle (4 ticks)
    // After 1 tick, output the ROMC command
    clock_tick(1);
    set_romc(romc);
    
    clock_tick(2);
    
    // Toggle WRITE on the last tick
    digitalWrite(WRITE_PIN, HIGH);
    clock_tick(1);
    uint8_t data = read_dbus(); // Data bus will be valid when WRITE is high
    digitalWrite(WRITE_PIN, LOW);
    
    return data;
}


// Execute one or more clock cycles
void clock_tick(uint8_t repeat) {
    for (uint8_t i = 0; i < repeat; i++) {
      digitalWrite(PHI_PIN, HIGH);
      delayMicroseconds(PHI / 2);
      digitalWrite(PHI_PIN, LOW);
      delayMicroseconds(PHI / 2);
    }
}

// Read the first 4K of cartridge ROM
void dump_cartridge(bool read_only) {
    // Set PC0 to the start of game ROM 0x0800
    if (read_only) {
        execute_in(0x08); // Load the data bus into both halves of PC0. Sets PC0 to 0xFFFF
        for (uint16_t i = 0; i < 0x801; i++) { // Increment PC0 until it's 0x0800
            execute_in(0x00);
        }
    } else {
        execute_out(0x14, 0x08); // Load the data bus into the high order byte of PC0. Sets PC0 to 0x08??
        execute_out(0x17, 0x00); // Load the data bus into the low order byte of PC0. Sets PC0 to 0x0800
    }

    // Read ROM data
    char dataString[4];
    char addrString[7];
    Serial.print("\n0000:");
    for (uint16_t addr = 0; addr < 0x1000; addr++) { // Assume 4K ROM
        sprintf(dataString, " %02X", execute_in(0x00));
        Serial.print(dataString);
        if ((addr & 0xF) == 0xF && addr != 0x1000 - 1) {
            sprintf(addrString, "\n%04X:", addr + 1 + 0x800);
            Serial.print(addrString);
        }
    }
    Serial.println("\nThat's All Folks!");
}
