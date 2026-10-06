<#
  Runs the QEMU emulation demos. Run it from the repo root (or anywhere, with the full path):

    .\emulation-tools\demo.ps1 hello      # screenshot 1: hello-world boots in QEMU
    .\emulation-tools\demo.ps1 mcu        # screenshot 2: real MCU firmware runs stably
    .\emulation-tools\demo.ps1 cantest    # screenshot 3: CAN does not work in QEMU
    .\emulation-tools\demo.ps1 twonode    # screenshot 4: MCU + vitals node on one CAN bus
    .\emulation-tools\demo.ps1 motor      # screenshot 5: firmware's motor status from the emulated H300
    .\emulation-tools\demo.ps1 pedal      # screenshot 6: pedal moves the motor
    .\emulation-tools\demo.ps1 sim        # screenshot 7: closed loop with the mock simulator

  Add -Rebuild to force a fresh build (needed after changing sdkconfig.mcu or any firmware code: the flash images
  are cached). Everything it creates (builds, test projects, flash images, logs) goes into emulation-tools\demo_tmp,
  which is git-ignored, so you can delete that folder at any time.
#>
param(
    [string]$Name = '',
    [switch]$Rebuild
)

$valid = 'hello', 'mcu', 'cantest', 'twonode', 'motor', 'pedal', 'sim'
if ($valid -notcontains $Name) {
    Write-Host ''
    Write-Host 'Pick which screenshot to recreate, for example:  .\emulation-tools\demo.ps1 hello' -ForegroundColor Yellow
    Write-Host ''
    Write-Host '  hello     screenshot 1  hello-world boots in QEMU'
    Write-Host '  mcu       screenshot 2  real MCU firmware runs stably'
    Write-Host '  cantest   screenshot 3  CAN does not work in QEMU'
    Write-Host '  twonode   screenshot 4  MCU + vitals node on one CAN bus'
    Write-Host '  motor     screenshot 5  firmware''s motor status from the emulated H300'
    Write-Host '  pedal     screenshot 6  pedal moves the motor'
    Write-Host '  sim       screenshot 7  closed loop with the mock simulator'
    Write-Host ''
    exit 0
}

$Here = $PSScriptRoot                       # this folder: <repo>\emulation-tools
$Repo = Split-Path $Here -Parent            # the ProveMila-embedded repo
$Tmp = Join-Path $Here 'demo_tmp'           # everything the demos create goes here (git-ignored)
New-Item -ItemType Directory -Force $Tmp | Out-Null

# Find ESP-IDF 5.5.x's PowerShell setup script and the QEMU that was installed with it (EIM installer layout)
$IdfProfile = Get-ChildItem 'C:\Espressif\tools\Microsoft.v5.5*.PowerShell_profile.ps1' -ErrorAction SilentlyContinue |
    Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty FullName
$Qemu = Get-ChildItem 'C:\Espressif\tools\tools\qemu-xtensa\*\qemu\bin\qemu-system-xtensa.exe' -ErrorAction SilentlyContinue |
    Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
if (-not $IdfProfile) { throw 'ESP-IDF 5.5.x setup script not found in C:\Espressif\tools (install ESP-IDF 5.5 with the EIM installer).' }
if (-not $Qemu) { throw 'qemu-system-xtensa.exe not found under C:\Espressif\tools\tools\qemu-xtensa.' }

& $IdfProfile *>$null
Get-Process qemu-system-xtensa -ErrorAction SilentlyContinue | Stop-Process -Force
$started = @()

function Say($text) { Write-Host $text -ForegroundColor Cyan }

function Merge-Image($buildDir, $out) {
    Push-Location $buildDir
    python -m esptool --chip esp32 merge_bin -o $out --fill-flash-size 4MB "@flash_args" *>$null
    Pop-Location
    if (-not (Test-Path $out)) { throw "could not create flash image $out" }
}

function Start-Qemu($image, $uart0, $uart1Port) {
    $a = '-M esp32 -m 4M -drive "file={0},if=mtd,format=raw" -nographic -serial "{1}"' -f $image, $uart0
    if ($uart1Port) { $a += (' -serial "tcp:127.0.0.1:{0},server,nowait"' -f $uart1Port) }
    $proc = Start-Process $Qemu -ArgumentList $a -PassThru -WindowStyle Hidden
    $script:started += $proc
    return $proc
}

function Start-Python($scriptName, $arguments, $outFile) {
    $a = '"{0}" {1}' -f (Join-Path $Here $scriptName), $arguments
    $proc = Start-Process python -ArgumentList $a -WorkingDirectory $Here -RedirectStandardOutput $outFile -PassThru -WindowStyle Hidden
    $script:started += $proc
    return $proc
}

function Ensure-McuImage {
    $img = Join-Path $Tmp 'mila_flash.bin'
    if ($Rebuild -or -not (Test-Path $img)) {
        Say 'Building the MCU firmware for QEMU (the first time takes a few minutes)...'
        $b = Join-Path $Tmp 'build_mcu'   # own build folder + sdkconfig, so the repo stays clean
        Push-Location $Repo
        idf.py -B $b -D SDKCONFIG="$b\sdkconfig" -D CMAKE_MCU_ESP_BUILD=ON -D CMAKE_SENSOR_ESP_BUILD=ON -D SENS_DIR=pedalSensor -D SDKCONFIG_DEFAULTS=sdkconfig.mcu -D CMAKE_QEMU_BUILD=ON set-target esp32 *>"$Tmp\build_mcu.log"
        if ($LASTEXITCODE -ne 0) { Pop-Location; throw "configure failed, see $Tmp\build_mcu.log" }
        idf.py -B $b build *>>"$Tmp\build_mcu.log"
        if ($LASTEXITCODE -ne 0) { Pop-Location; throw "build failed, see $Tmp\build_mcu.log" }
        Pop-Location
        Merge-Image $b $img
    }
    return $img
}

function Ensure-VitalsImage {
    $img = Join-Path $Tmp 'node_vitals.bin'
    if ($Rebuild -or -not (Test-Path $img)) {
        Say 'Building the vitals node for QEMU (the first time takes a few minutes)...'
        $b = Join-Path $Tmp 'build_vitals'
        Push-Location $Repo
        idf.py -B $b -D SDKCONFIG="$b\sdkconfig" -D SDKCONFIG_DEFAULTS=sdkconfig.mcu -D CMAKE_VITALS_ESP_BUILD=ON -D CMAKE_QEMU_BUILD=ON set-target esp32 *>"$Tmp\build_vitals.log"
        if ($LASTEXITCODE -ne 0) { Pop-Location; throw "configure failed, see $Tmp\build_vitals.log" }
        idf.py -B $b build *>>"$Tmp\build_vitals.log"
        if ($LASTEXITCODE -ne 0) { Pop-Location; throw "build failed, see $Tmp\build_vitals.log" }
        Pop-Location
        Merge-Image $b $img
    }
    return $img
}

function Ensure-TestProject($projectName, $mainFile) {
    $dir = Join-Path $Tmp $projectName
    $img = Join-Path $Tmp "$projectName.bin"
    if ($Rebuild -or -not (Test-Path $img)) {
        Say "Building the $projectName test project..."
        Push-Location $Tmp
        if (-not (Test-Path $dir)) { idf.py create-project $projectName *>$null }
        Set-Location $dir
        if ($mainFile) { Copy-Item (Join-Path $Here $mainFile) (Join-Path $dir "main\$projectName.c") -Force }
        idf.py set-target esp32 *>"$Tmp\build_$projectName.log"
        idf.py build *>>"$Tmp\build_$projectName.log"
        $ok = ($LASTEXITCODE -eq 0)
        Pop-Location
        if (-not $ok) { throw "build failed, see $Tmp\build_$projectName.log" }
        Merge-Image (Join-Path $dir 'build') $img
    }
    return $img
}

try {
    switch ($Name) {
        'hello' {
            $img = Ensure-TestProject 'hello55' $null
            $log = Join-Path $Tmp 'hello.log'
            $null = Start-Qemu $img "file:$log" $null
            Start-Sleep 10
            $lines = Get-Content $log
            $start = ($lines | Select-String '2nd stage bootloader' | Select-Object -First 1).LineNumber
            $end = ($lines | Select-String 'Chip rev:' | Select-Object -Last 1).LineNumber
            Say '=== SCREENSHOT 1: hello-world booting in QEMU (ESP-IDF v5.5.5) ==='
            $lines[($start - 1)..($end - 1)]
        }
        'mcu' {
            $img = Ensure-McuImage
            $log = Join-Path $Tmp 'mcu.log'
            $null = Start-Qemu $img "file:$log" 5001
            Start-Sleep 20
            $lines = Get-Content $log
            $boots = ($lines | Select-String '2nd stage bootloader').Count
            $start = ($lines | Select-String 'Hello, minimal app starting' | Select-Object -First 1).LineNumber
            $end = ($lines | Select-String 'Returned from app_main' | Select-Object -First 1).LineNumber
            Say '=== SCREENSHOT 2: real MCU firmware running stably after the fix ==='
            # the pedal debug prints share the console and would clutter the screenshot, so leave them out
            # a real log line ("I (1234) tag: text") is always kept; if a pedal print got glued in front of it, only that is cut off
            $lines[($start - 1)..($end - 1)] | ForEach-Object {
                if ($_ -match '[IEW] \(\d+\) \w+:') { $_.Substring($_.IndexOf($Matches[0])) }
                elseif ($_ -notmatch '^\s*$' -and $_ -notmatch 'collecting|collect$|^ing pedal|__|\|_|^\s+\||\b(One|Two): -?\d+|PowermV') { $_ }
            }
            Say ("boots in 20 s: {0}   crashes: {1}" -f $boots, ($lines | Select-String 'assert failed|Backtrace').Count)
        }
        'cantest' {
            $img = Ensure-TestProject 'can_test' 'can_test.c'
            $log = Join-Path $Tmp 'cantest.log'
            $null = Start-Qemu $img "file:$log" $null
            Start-Sleep 25
            Say '=== SCREENSHOT 3: CAN (TWAI) does not work in QEMU ==='
            Get-Content $log | Select-String 'CANTEST' | ForEach-Object { $_.Line }
        }
        'twonode' {
            $mcu = Ensure-McuImage
            $vit = Ensure-VitalsImage
            $null = Start-Qemu $mcu "file:$Tmp\two_mcu.log" 5001
            $null = Start-Qemu $vit "file:$Tmp\two_vitals.log" 5002
            Start-Sleep 2
            $relay = Join-Path $Tmp 'two_relay.txt'
            Say 'Running the relay for 40 s (frames from both chips)...'
            $p = Start-Python 'can_relay.py' '5001,5002 40' $relay
            $p.WaitForExit()
            $text = Get-Content $relay
            Say '=== SCREENSHOT 4: two emulated ESPs on one CAN bus ==='
            Say '--- most frequent frames on the bus (count, source port, CAN id) ---'
            $text | Where-Object { $_ -match ' port ' } | ForEach-Object { $_ -replace '^\[[0-9:]+\] ', '' } |
                Group-Object | Sort-Object Count -Descending | Select-Object -First 8 | ForEach-Object { '{0,5} x  {1}' -f $_.Count, $_.Name }
            Say '--- relay summary ---'
            $text | Where-Object { $_ -match '^relay: frames' } | ForEach-Object { $_ -replace "^(.{140}).*", '$1 ...' }
            Say '--- vitals node log (it receives the MCU''s answer) ---'
            Get-Content "$Tmp\two_vitals.log" | Select-String 'sent HB|recieved Pong' | Select-Object -First 3 | ForEach-Object { $_.Line }
        }
        'motor' {
            $img = Ensure-McuImage
            $null = Start-Qemu $img 'tcp:127.0.0.1:5000,server,nowait' 5001
            Start-Sleep 2
            $null = Start-Python 'can_relay.py' '5001 60 --motor --quiet' (Join-Path $Tmp 'motor_relay.txt')
            Say '=== SCREENSHOT 5: the firmware''s own view of the emulated motor controller (takes about 30 s) ==='
            python (Join-Path $Here 'console_probe.py') 5000 14 motor_speed motor_power motor_safety
        }
        'pedal' {
            $img = Ensure-McuImage
            $null = Start-Qemu $img 'tcp:127.0.0.1:5000,server,nowait' 5001
            Start-Sleep 2
            $null = Start-Python 'can_relay.py' '5001 80 --motor --quiet --pedal "50@0,100@14,20@26,0@38"' (Join-Path $Tmp 'pedal_relay.txt')
            Say '=== SCREENSHOT 6: the emulated pedal moving the motor (takes about 60 s) ==='
            python (Join-Path $Here 'console_probe.py') 5000 12 series 46
        }
        'sim' {
            $img = Ensure-McuImage
            $null = Start-Qemu $img 'tcp:127.0.0.1:5000,server,nowait' 5001
            Start-Sleep 2
            $null = Start-Python 'can_relay.py' '5001 90 --motor --quiet --sim-port 6000' (Join-Path $Tmp 'sim_relay.txt')
            Start-Sleep 2
            Say '=== SCREENSHOT 7: closed loop with the mock simulator (takes about 1 minute) ==='
            python (Join-Path $Here 'mock_sim.py') 6000 56 '50@0,100@10,20@22,0@34'
        }
    }
}
finally {
    foreach ($proc in $started) { if ($proc -and -not $proc.HasExited) { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue } }
    Get-Process qemu-system-xtensa -ErrorAction SilentlyContinue | Stop-Process -Force
}
