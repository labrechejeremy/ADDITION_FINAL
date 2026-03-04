$ErrorActionPreference = 'Stop'

function Send-RpcCommand {
    param([string]$Command)
    try {
        $client = New-Object System.Net.Sockets.TcpClient
        $client.ReceiveTimeout = 3000
        $client.SendTimeout = 3000
        $client.Connect('127.0.0.1', 8545)
        
        $stream = $client.GetStream()
        $writer = New-Object System.IO.StreamWriter($stream)
        $reader = New-Object System.IO.StreamReader($stream)
        $writer.AutoFlush = $true
        
        $writer.WriteLine($Command)
        $response = $reader.ReadLine()
        
        $reader.Dispose()
        $writer.Dispose()
        $stream.Dispose()
        return $response
    }
    catch {
        return $null
    }
    finally {
        $client.Dispose()
    }
}

Write-Host "=== CREATION DE WALLET ===" -ForegroundColor Cyan

$wallet = Send-RpcCommand 'createwallet'
if ($wallet) {
    Write-Host "✅ Wallet créé:" -ForegroundColor Green
    Write-Host $wallet -ForegroundColor White
    
    # Extraire l'adresse
    if ($wallet -match 'address=([^\s]+)') {
        $addr = $matches[1]
        Write-Host "`n📍 ADRESSE: $addr" -ForegroundColor Yellow
    }
} else {
    Write-Host "❌ Erreur: daemon non accessible sur 127.0.0.1:8545" -ForegroundColor Red
    exit 1
}
