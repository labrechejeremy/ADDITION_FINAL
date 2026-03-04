param(
    [string]$MinerAddress = "miner1",
    [string]$RpcHost = "127.0.0.1",
    [int]$RpcPort = 8545,
    [int]$IntervalMs = 1200
)

$ErrorActionPreference = 'Stop'

function Invoke-RpcLine {
    param([string]$Command)

    $client = New-Object System.Net.Sockets.TcpClient
    try {
        $client.ReceiveTimeout = 4000
        $client.SendTimeout = 4000
        $client.Connect($RpcHost, $RpcPort)

        $stream = $client.GetStream()
        $writer = New-Object System.IO.StreamWriter($stream)
        $writer.AutoFlush = $true
        $reader = New-Object System.IO.StreamReader($stream)

        $writer.WriteLine($Command)
        $resp = $reader.ReadLine()

        $reader.Dispose()
        $writer.Dispose()
        $stream.Dispose()
        return $resp
    }
    finally {
        $client.Dispose()
    }
}

function Parse-KvLine {
    param([string]$Line)
    $map = @{}
    if (-not $Line) { return $map }
    foreach ($item in $Line.Split(' ')) {
        if ($item -like '*=*') {
            $kv = $item.Split('=', 2)
            $map[$kv[0]] = $kv[1]
        }
    }
    return $map
}

Write-Host "[mine-loop] target=$MinerAddress rpc=$RpcHost`:$RpcPort interval=${IntervalMs}ms"
Write-Host "[mine-loop] Press Ctrl+C to stop."

try {
    $pre = Invoke-RpcLine "getinfo"
    if (-not $pre) {
        throw "RPC returned empty response"
    }
    Write-Host "[mine-loop] node ready: $pre"
}
catch {
    Write-Host "[mine-loop][fatal] RPC not reachable: $_"
    exit 2
}

while ($true) {
    try {
        $mine = Invoke-RpcLine "mine $MinerAddress"
        $infoRaw = Invoke-RpcLine "getinfo"
        $balRaw = Invoke-RpcLine "getbalance $MinerAddress"

        $info = Parse-KvLine $infoRaw
        $h = if ($info.ContainsKey('height')) { $info['height'] } else { '-' }
        $r = if ($info.ContainsKey('next_reward')) { $info['next_reward'] } else { '-' }

        $ts = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        Write-Host "[$ts] mined=[$mine] height=$h next_reward=$r balance($MinerAddress)=$balRaw"
    }
    catch {
        $ts = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        Write-Host "[$ts] [warn] mining call failed: $_"
    }

    Start-Sleep -Milliseconds $IntervalMs
}
