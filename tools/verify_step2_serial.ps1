[CmdletBinding()]
param(
    [string]$Port,
    [ValidateRange(1, 60)]
    [int]$DurationSeconds = 8,
    [string]$LogPath
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot

if (-not $LogPath) {
    $LogPath = Join-Path $projectRoot 'artifacts\step2-serial.log'
}

$availablePorts = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
if (-not $Port) {
    if ($availablePorts.Count -eq 0) {
        throw 'No COM port detected. Connect the USB-to-TTL adapter, then run this script again.'
    }

    if ($availablePorts.Count -gt 1) {
        throw "Multiple COM ports detected: $($availablePorts -join ', '). Select one with -Port COMx."
    }

    $Port = $availablePorts[0]
} elseif ($Port -notin $availablePorts) {
    throw "Serial port $Port is not present. Available ports: $($availablePorts -join ', ')."
}

$serial = [System.IO.Ports.SerialPort]::new($Port, 115200, 'None', 8, 'One')
$serial.Handshake = 'None'
$serial.DtrEnable = $false
$serial.RtsEnable = $false
$serial.ReadTimeout = 100
$serial.NewLine = "`n"

$commands = @('', 'uname -a', 'free', 'ps', 'mount', 'ls /proc')
$output = [System.Text.StringBuilder]::new()

try {
    Write-Host "Opening $Port at 115200-8-N-1. Press the C-board RESET button now if no prompt appears."
    $serial.Open()
    Start-Sleep -Milliseconds 500
    $serial.DiscardInBuffer()

    foreach ($command in $commands) {
        $serial.WriteLine($command)
        Start-Sleep -Milliseconds 250
    }

    $deadline = [DateTime]::UtcNow.AddSeconds($DurationSeconds)
    while ([DateTime]::UtcNow -lt $deadline) {
        try {
            $chunk = $serial.ReadExisting()
            if ($chunk.Length -gt 0) {
                [void]$output.Append($chunk)
                Write-Host -NoNewline $chunk
            }
        } catch [System.TimeoutException] {
        }

        Start-Sleep -Milliseconds 50
    }
} finally {
    if ($serial.IsOpen) {
        $serial.Close()
    }
    $serial.Dispose()
}

$logDirectory = Split-Path -Parent $LogPath
if ($logDirectory) {
    New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
}
$text = $output.ToString()
[System.IO.File]::WriteAllText($LogPath, $text, [System.Text.UTF8Encoding]::new($false))

if ($text -notmatch 'nsh>') {
    throw "The serial port opened, but no NSH prompt was received. Check TX/RX crossing and press RESET. Log: $LogPath"
}

Write-Host "`n[PASS] NSH responded on $Port. Step 2 serial log: $LogPath"
