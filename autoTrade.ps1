################################
### Automated trading module ###
################################

<#  [NOTE]
    This thing will execute orders based on JSON data
#>

# ---------------------------------
# Import external modules
# ---------------------------------
. "./modules/sqlBackend.ps1"
. "./modules/APIinterface.ps1"

# ---------------------------------
# VARS
# ---------------------------------
$QuoteAssetFilterList = "./filter/QuoteAssetFilterList.txt"
$logFile = "./log/autoTrade_$(Get-Date -Format "dd-MM-yyyy_HH-mm-ss").txt"

# ---------------------------------
# FUNCTIONS
# ---------------------------------
function log {
    param(
        [string]$in,
        [string]$logSymbol = "i"
    )
    Write-Output "[$(Get-Date -Format 'HH:mm:ss')] ($logSymbol) $in" >> $logfile
}

# ---------------------------------
# MAIN
# ---------------------------------

Write-Host "CAUTION: No verbose logging in console window!" -fore Yellow 
Write-Host "Please consult '" -NoNewLine -fore yellow
    Write-Host ${logFile} -NoNewLine -fore cyan
    Write-host "'." -fore Yellow
Write-Host ""

# Create log dir if it does not exist yet
If (!(Test-Path "./log")) {
    mkdir ".\log" | out-file
}

Write-Output "New session started on $(Get-Date)" > $logfile
log "Checking if Binance DB already exists..."
if (Create-BinanceDB -eq 1) {
    log " > DB already exists."
} else {
    log " > DB not found." "X"
    Write-Host "ERROR:" -fore red -back Black
    Write-Host "Binance DB does not exist. \nPlease create" -fore red -back Black
    exit
}

Write-Host "Querying binance status..." -fore yellow
log "Querying binance status..."
Get-BinanceSysStatus
if ($exchange_maintenance) {
    log "Binance is under maintenance." "X"
    Write-Host "[X] ERROR:" -fore Red -back black
    Write-Host "    Binance is under maintenance." -fore red
    exit
} else {
    log " > Binance is accessible."
    Write-Host " > Binance accessible." -fore green
}

[datetime]$now = [datetime]::ParseExact((Get-Date -Format 'dd.MM.yyyy HH:mm:ss'),"dd.MM.yyyy HH:mm:ss",$null)

# Create list of potential orders
if (!(Test-Path ".\automation")) {
    log "'./automation' does not yet exist, creating..."
    mkdir "./automation" | Out-Null
}

log "Probing for order data..."
$marketOrders = Get-ChildItem "./automation" | where {$_ -like "*order_market*.json"}
log "> Found $($marketOrders.count) MARKET order(s)."
$limitOrders = Get-ChildItem "./automation" | where {$_ -like "*order_limit*.json"}
log "> Found $($limitOrders.count) MARKET order(s)."