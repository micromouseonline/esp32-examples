import csv
import os
import re
import subprocess
import time
import serial
import serial.tools.list_ports

CSV_FILE = "esp32_inventory.csv"
CSV_HEADERS = ["MAC Address", "Name", "Chip Type", "Flash Size", "RAM", "PSRAM"]

def find_esp32_port():
    """Attempts to auto-detect the serial port of the connected ESP32."""
    ports = list(serial.tools.list_ports.comports())
    keywords = ["CP210", "CH340", "CH341", "FTDI", "UART", "USB", "Espressif", "ACM"]
    
    detected_ports = []
    for port in ports:
        if any(kw.lower() in (port.description + port.device).lower() for kw in keywords):
            detected_ports.append(port.device)
            
    if not detected_ports:
        return None
    if len(detected_ports) == 1:
        return detected_ports[0]
    
    print("\nMultiple potential devices found:")
    for i, p in enumerate(detected_ports):
        print(f"[{i}] {p}")
    choice = input("Select the port index: ")
    try:
        return detected_ports[int(choice)]
    except (ValueError, IndexError):
        return None

def reset_native_usb(port):
    """Toggles DTR/RTS to kick the native USB CDC controller out of a stuck loop."""
    try:
        ser = serial.Serial(port)
        ser.dtr = False
        ser.rts = False
        time.sleep(0.1)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        ser.close()
        time.sleep(0.2)
    except Exception:
        pass # If it's locked solid, ignore and let esptool try its own reset sequence

def get_esp32_details(port):
    """Runs esptool with retries and lower baud rate to counter S3 busy errors."""
    output = None
    flash_output = None
    
    for attempt in range(1, 4):
        print(f"Connecting to ESP32 on {port} (Attempt {attempt}/3)...")
        reset_native_usb(port)
        
        try:
            # Using --before no_reset or default, but forcing low baud rate
            result = subprocess.run(
                ["esptool", "--port", port, "--baud", "115200", "chip_id"],
                capture_output=True, text=True, timeout=5, check=True
            )
            output = result.stdout
            
            flash_result = subprocess.run(
                ["esptool", "--port", port, "--baud", "115200", "flash_id"],
                capture_output=True, text=True, timeout=5, check=True
            )
            flash_output = flash_result.stdout
            break 
            
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
            print("  Port busy or failed to sync. Retrying...")
            time.sleep(1.0)
            
    if not output or not flash_output:
        print("\n❌ Error: Failed to communicate with ESP32.")
        print("💡 Hardware Fix: Press and HOLD the 'BOOT' button, tap the 'RST' button, then release 'BOOT'. Now try again.")
        return None

    # Parse details
    mac = re.search(r"MAC:\s*([0-9a-fA-F:]+)", output)
    chip_type = re.search(r"Detecting chip type\.\.\.\s*(.+)", output)
    flash_size = re.search(r"Detected flash size:\s*(.+)", flash_output)
    features = re.search(r"Features:\s*(.+)", output)
    
    mac_str = mac.group(1).upper() if mac else "UNKNOWN"
    chip_str = chip_type.group(1).strip() if chip_type else "ESP32"
    flash_str = flash_size.group(1).strip() if flash_size else "UNKNOWN"
    
    ram_str = "520 KB SRAM"
    if "ESP32-S3" in chip_str:
        ram_str = "512 KB SRAM"
    elif "ESP32-C3" in chip_str:
        ram_str = "400 KB SRAM"
        
    features_str = features.group(1) if features else ""
    psram_str = "Embedded PSRAM" if "PSRAM" in features_str else "None detected"

    return {
        "mac": mac_str,
        "chip": chip_str,
        "flash": flash_str,
        "ram": ram_str,
        "psram": psram_str
    }

def is_duplicate(mac, filename):
    if not os.path.exists(filename):
        return False
    with open(filename, mode='r', newline='', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row.get("MAC Address") == mac:
                return row.get("Name")
    return False

def init_csv(filename):
    if not os.path.exists(filename):
        with open(filename, mode='w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(CSV_HEADERS)

def catalog_single_board():
    port = find_esp32_port()
    if not port:
        port = input("Could not auto-detect port. Enter manually (or leave blank to cancel): ").strip()
        if not port:
            return False

    details = get_esp32_details(port)
    if not details:
        return False

    print(f"\n--- Detected Board ---")
    print(f"MAC Address : {details['mac']}")
    print(f"Chip Type   : {details['chip']}")
    print(f"Flash Size  : {details['flash']}")
    print(f"Base RAM    : {details['ram']}")
    print(f"PSRAM       : {details['psram']}")
    print("----------------------")

    existing_name = is_duplicate(details['mac'], CSV_FILE)
    if existing_name:
        print(f"\n⚠️ WARNING: Already cataloged as '{existing_name}'!")
        choice = input("Overwrite? (y/N): ").strip().lower()
        if choice != 'y':
            print("Skipping.")
            return True

    while True:
        name = input("\nEnter name (max 20 chars): ").strip()
        if 0 < len(name) <= 20:
            break
        print("Invalid name length.")

    with open(CSV_FILE, mode='a', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow([details['mac'], name, details['chip'], details['flash'], details['ram'], details['psram']])
        
    print(f"✅ Added '{name}'.")
    return True

def main():
    init_csv(CSV_FILE)
    print("========================================")
    print("       ESP32 Inventory Cataloger        ")
    print("========================================")
    
    while True:
        print("\n--- Ready for Next Board ---")
        user_input = input("👉 Plug in a board and press ENTER to scan (or type 'q' to quit): ").strip().lower()
        
        if user_input == 'q':
            break
            
        catalog_single_board()
            
    print("\nInventory complete. Saved to:", os.path.abspath(CSV_FILE))

if __name__ == "__main__":
    main()