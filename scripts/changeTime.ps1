$uri = "http://192.168.1.137/api/time"

$b = @{
    "IsDST" = $false
} | ConvertTo-Json

Invoke-RestMethod $uri -Body $b -ContentType "application/json" -Method POST
