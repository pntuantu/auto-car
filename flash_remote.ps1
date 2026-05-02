# $PI_HOST = "tu@192.168.2.10"
# $PI_FW_DIR = "/home/tu/fw"

# if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
#     Write-Error "Khong tim thay idf.py. Hay mo ESP-IDF PowerShell hoac chay export.ps1 truoc."
#     exit 1
# }

# idf.py build
# if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# $APP_BIN = Get-ChildItem build\*.bin |
#     Where-Object {
#         $_.Name -notmatch "^bootloader\.bin$" -and
#         $_.Name -notmatch "^partition-table\.bin$" -and
#         $_.Name -notmatch "^ota_data_initial\.bin$"
#     } |
#     Select-Object -First 1

# if (-not $APP_BIN) {
#     Write-Error "Khong tim thay app .bin trong thu muc build"
#     exit 1
# }

# $APP_NAME = $APP_BIN.Name

# ssh $PI_HOST "mkdir -p $PI_FW_DIR"
# if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# scp `
#     build/bootloader/bootloader.bin `
#     build/partition_table/partition-table.bin `
#     $APP_BIN.FullName `
#     "${PI_HOST}:$PI_FW_DIR/"
# if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# try {
#     Write-Host "Stopping ser2net on Pi..."
#     ssh $PI_HOST "sudo systemctl stop ser2net && sudo systemctl is-active ser2net || true"
#     if ($LASTEXITCODE -ne 0) { throw "Khong dung duoc ser2net tren Pi" }

#     Write-Host "Flashing ESP32-S3 from Pi..."
#     ssh $PI_HOST "python3 -m esptool --chip esp32s3 --port /dev/ttyACM0 --baud 921600 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 2MB 0x0 $PI_FW_DIR/bootloader.bin 0x10000 $PI_FW_DIR/$APP_NAME 0x8000 $PI_FW_DIR/partition-table.bin"
#     if ($LASTEXITCODE -ne 0) { throw "Nap firmware that bai" }
# }
# finally {
#     Write-Host "Starting ser2net on Pi..."
#     ssh $PI_HOST "sudo systemctl start ser2net && sudo systemctl is-active ser2net"
# }


# #telnet 192.168.2.10 3333
# #powershell -ExecutionPolicy Bypass -File .\flash_remote.ps1





$PI_HOST = "tu@192.168.2.9"
$PI_FW_DIR = "/home/tu/fw"

if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    Write-Error "Khong tim thay idf.py. Hay mo ESP-IDF PowerShell hoac chay export.ps1 truoc."
    exit 1
}

idf.py build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$APP_BIN = Get-ChildItem build\*.bin |
    Where-Object {
        $_.Name -notmatch "^bootloader\.bin$" -and
        $_.Name -notmatch "^partition-table\.bin$" -and
        $_.Name -notmatch "^ota_data_initial\.bin$"
    } |
    Select-Object -First 1

if (-not $APP_BIN) {
    Write-Error "Khong tim thay app .bin trong thu muc build"
    exit 1
}

$APP_NAME = $APP_BIN.Name

ssh $PI_HOST "mkdir -p $PI_FW_DIR"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

scp `
    build/bootloader/bootloader.bin `
    build/partition_table/partition-table.bin `
    $APP_BIN.FullName `
    "${PI_HOST}:$PI_FW_DIR/"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

try {
    Write-Host "Stopping ser2net on Pi..."
    ssh $PI_HOST "sudo systemctl stop ser2net && sudo systemctl is-active ser2net || true"
    if ($LASTEXITCODE -ne 0) { throw 'Khong dung duoc ser2net tren Pi' }

    Write-Host "Detecting ESP32-S3 USB port on Pi..."
    $PI_PORT = ssh $PI_HOST "ls -1 /dev/serial/by-id/*Espressif* 2>/dev/null | head -n 1"
    $PI_PORT = $PI_PORT.Trim()

    if (-not $PI_PORT) {
        $PI_PORT = ssh $PI_HOST "ls -1 /dev/ttyACM* 2>/dev/null | head -n 1"
        $PI_PORT = $PI_PORT.Trim()
    }

    if (-not $PI_PORT) {
        throw "Khong tim thay cong USB cua ESP32-S3 tren Pi"
    }

    Write-Host "Flashing ESP32-S3 from Pi via USB port $PI_PORT ..."
    ssh $PI_HOST "python3 -m esptool --chip esp32s3 --port $PI_PORT --baud 921600 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 2MB 0x0 $PI_FW_DIR/bootloader.bin 0x8000 $PI_FW_DIR/partition-table.bin 0x10000 $PI_FW_DIR/$APP_NAME"
    if ($LASTEXITCODE -ne 0) { throw 'Nap firmware that bai' }
}
finally {
    Write-Host "Starting ser2net on Pi..."
    ssh $PI_HOST "sudo systemctl start ser2net && sudo systemctl is-active ser2net"
}