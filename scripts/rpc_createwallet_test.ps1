param(
    [string]$RpcHost = "127.0.0.1",
    [int]$RpcPort = 8545
)

$client = New-Object System.Net.Sockets.TcpClient
$client.ReceiveTimeout = 5000
$client.SendTimeout = 5000

try {
    $client.Connect($RpcHost, $RpcPort)
    $stream = $client.GetStream()
    $writer = New-Object System.IO.StreamWriter($stream)
    $writer.AutoFlush = $true
    $reader = New-Object System.IO.StreamReader($stream)

    $writer.WriteLine("createwallet")
    $resp = $reader.ReadLine()

    if ([string]::IsNullOrWhiteSpace($resp)) {
        Write-Host "[FAIL] Empty response from createwallet"
        exit 2
    }

    Write-Host "[OK] createwallet response:"
    Write-Host $resp

    if ($resp -notlike "address=*") {
        Write-Host "[FAIL] Unexpected format"
        exit 3
    }

    exit 0
}
catch {
    Write-Host "[FAIL] RPC error: $($_.Exception.Message)"
    exit 1
}
finally {
    if ($reader) { $reader.Dispose() }
    if ($writer) { $writer.Dispose() }
    if ($stream) { $stream.Dispose() }
    $client.Dispose()
}
