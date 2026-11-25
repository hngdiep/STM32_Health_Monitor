# Live RTC synchronization for BOTH normal and SD_TEST firmware.
# Must be executed after flashing exactly the firmware.elf currently in build/.
# Local Windows wall-clock time, seconds since 2000-01-01 (not Unix).
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$elf = Join-Path $root 'build\firmware.elf'
$packageRoot = Join-Path $env:USERPROFILE '.platformio\packages\tool-openocd'
$oc = Join-Path $packageRoot 'bin\openocd.exe'
if (!(Test-Path $elf)) { throw 'Firmware ELF missing. Build and flash normal or SD_TEST firmware first.' }
if (!(Test-Path $oc)) { throw "OpenOCD not found: $oc" }
$stlinkCfg = Get-ChildItem -Path $packageRoot -Recurse -File -Filter 'stlink.cfg' -ErrorAction SilentlyContinue |
    Where-Object { $_.Directory.Name -eq 'interface' } | Select-Object -First 1
if (!$stlinkCfg) { throw 'Cannot find interface/stlink.cfg in PlatformIO OpenOCD package' }
$scripts = $stlinkCfg.Directory.Parent.FullName

$nm = & arm-none-eabi-nm $elf
if ($LASTEXITCODE -ne 0) { throw 'arm-none-eabi-nm failed' }
function Get-SymbolAddress([string]$name) {
    $matchesFound = @($nm | Where-Object {
        $_ -match ('^[0-9a-fA-F]+\s+[BbDd]\s+' + [regex]::Escape($name) + '$')
    })
    if ($matchesFound.Count -ne 1) { throw "Missing symbol: $name (build both modes with sd_rtc_sync.c)" }
    return '0x' + ($matchesFound[0] -split '\s+')[0]
}
$timeAddress = Get-SymbolAddress 'g_sd_sync_seconds'
$requestAddress = Get-SymbolAddress 'g_sd_sync_request'
$resultAddress = Get-SymbolAddress 'g_sd_sync_result'

$now = Get-Date
$epoch = [datetime]::new(2000, 1, 1, 0, 0, 0)
$seconds = [uint32][math]::Floor(($now - $epoch).TotalSeconds)
Write-Host ("PC local datetime: " + $now.ToString('yyyy-MM-dd HH:mm:ss'))
Write-Host "RTC seconds since 2000: $seconds"
Write-Host "RTC request address: $requestAddress"
Write-Host "RTC time address: $timeAddress"
Write-Host "RTC result address: $resultAddress"
Write-Host "OpenOCD scripts: $scripts"

# Result=0 (pending) -> write value -> request=1. Never reset MCU here.
$command = "adapter speed 50; init; mww $resultAddress 0; mww $timeAddress $seconds; mww $requestAddress 1; shutdown"
& $oc -s $scripts -f interface/stlink.cfg -f target/stm32f1x.cfg -c $command
if ($LASTEXITCODE -ne 0) { throw 'OpenOCD could not send the sync request' }

Start-Sleep -Milliseconds 700

# OpenOCD can print Tcl read_memory output to STDERR on Windows.
# A PowerShell pipeline captures only STDOUT, leaving $output empty.
# Capture BOTH streams in files without changing the native command's error behavior.
$tempPrefix = Join-Path ([System.IO.Path]::GetTempPath()) ("rtc_readback_" + [guid]::NewGuid().ToString('N'))
$stdoutFile = $tempPrefix + '.stdout.txt'
$stderrFile = $tempPrefix + '.stderr.txt'
try {
    $readCommand = "adapter speed 50; init; echo [read_memory $resultAddress 32 3]; shutdown"
    $arguments = '-s "{0}" -f interface/stlink.cfg -f target/stm32f1x.cfg -c "{1}"' -f $scripts, $readCommand
    $process = Start-Process -FilePath $oc -ArgumentList $arguments -NoNewWindow -Wait -PassThru `
        -RedirectStandardOutput $stdoutFile -RedirectStandardError $stderrFile
    $output = (Get-Content -Path $stdoutFile -Raw) + "`n" + (Get-Content -Path $stderrFile -Raw)
    if ($process.ExitCode -ne 0) { throw "OpenOCD readback failed (exit $($process.ExitCode)):`n$output" }
} finally {
    Remove-Item -Path $stdoutFile, $stderrFile -Force -ErrorAction SilentlyContinue
}
$pattern = '(?m)^\s*0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s*$'
$m = [regex]::Match($output, $pattern)
if (!$m.Success) { throw "Cannot parse RTC readback:`n$output" }
$result = [convert]::ToUInt32($m.Groups[1].Value, 16)
$request = [convert]::ToUInt32($m.Groups[2].Value, 16)
$written = [convert]::ToUInt32($m.Groups[3].Value, 16)
Write-Host "Readback result=$result request=$request time=$written"
if ($result -ne 1 -or $request -ne 0 -or $written -ne $seconds) {
    throw 'RTC was NOT confirmed synchronized. Check flashed ELF and firmware task.'
}
Write-Host 'RTC SYNCHRONIZED successfully on running STM32.'
